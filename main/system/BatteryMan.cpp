#include "BatteryMan.hpp"
#include <algorithm>  // for std::sort
#include <array>
#include <cstring>
#include "HAL.hpp"
#include "globals.hpp"
#include "math.h"
static BluemanInterface& blue = getBlueman();
static PicmanInterface& pic = getPicman();

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define NO_OF_SAMPLES 32  // Multisampling

#define BATMAN_BATT_INT_RES_MO 420.
#define BATMAN_BATT_READ_RES 18.7
#define BATMAN_BATT_OTH_RES 100.
#define BATMAN_BATT_NB_CELLS 4.

#define BATMAN_BATT_AVG_CURRENT 0.31  // arbitrary value based on oberservations
#define SOC_BUFFER_LEN 7

static HAL& hal = getHAL();
static const char LTAG[] = "Batman";
static std::array<int, SOC_BUFFER_LEN> soc_buffer;

/** @brief Inverts a value read on a resistor divider
 *
 * @param R1 The read resistor
 * @param R2 The other resistor
 * @param reading The value read
 * @return The value on the total resistor
 */
static float _easyResDiv(float R1, float R2, float reading) {
    assert(R1);

    float res = (R1 + R2) * reading / R1;

    return res;
}

/** @brief Computes the SOC of a 4.2V LiPo battery from its voltage
 *
 * @param tensionValue_v The value of the loaded voltage of the battery (in
 * volts)
 * @param current The current going into the battery. Negative if current is
 * goint out (in amps)
 * @param int_res_mo Internal resistance of the battery (in milli ohms)
 * @return The SOC in percent, -1 is an invalid value
 */
static float _percentageBatt(float tensionValue_v,
                             float current_a,
                             float int_res_mo) {
    float fpercentage = 0.;

    // correction battery voltage drop due to internal resistance
    tensionValue_v -= current_a * int_res_mo / 1000.;

    // calculation of the SOC
    if (tensionValue_v > 4.2) {
        fpercentage = 100.;
    } else if (tensionValue_v > 3.78) {
        fpercentage = 536.24 * tensionValue_v * tensionValue_v * tensionValue_v;
        fpercentage -= 6723.8 * tensionValue_v * tensionValue_v;
        fpercentage += 28186 * tensionValue_v - 39402;

        if (fpercentage > 100.)
            fpercentage = 100.;

    } else if (tensionValue_v > 3.2) {
        fpercentage = pow(10, -11.4) * pow(tensionValue_v, 22.315);

    } else if (tensionValue_v > 0.) {
        fpercentage = 0.;

    } else {
        fpercentage = -1.;
    }

    return fpercentage;
}

static int _mode_of_array(std::array<int, SOC_BUFFER_LEN> array) {
    int number = array[0];
    int mode = number;
    int count = 1;
    int countMode = 1;
    std::array<int, SOC_BUFFER_LEN> copy_array;

    copy_array = array;
    std::sort(copy_array.begin(), copy_array.end());

    for (uint8_t i = 1; i < array.size(); i++) {
        if (copy_array[i] == number)
            ++count;
        else {
            if (count > countMode) {
                countMode = count;
                mode = number;
            }
            count = 1;  // reset count for the new number
            number = copy_array[i];
        }
    }
    // check for the last value
    if (count > countMode) {
        countMode = count;
        mode = number;
    }

    return mode;
}

BatteryMan::BatteryMan() : m_batt_soc(0), m_bq_state(BQState::BQIdle) {}

void BatteryMan::task() {

    this->delay(2000);

    while (1) {
        // check BQ24650
        this->determine_bq_state();

        // update internal SOC
        this->measure_soc();

        // returns a more stable SOC
        this->mode_of_rbuffer();

        // send state to PIC
        this->update_pic_soc();

        // delay task
        this->delay(BATTERY_MANAGER_PERIOD_MS);
    }

    this->remove();
}

void BatteryMan::start(void) {
    // create task
    this->create("battery man", 2500, 10);
}

uint8_t BatteryMan::measure_soc(void) {
    uint32_t adc_reading = 0;
    float cell_voltage_v = 0;
    float perc_batt = 0;

    LOGD(LTAG, "Measuring ADC...");

    // sample
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += hal.adc1->get_raw();
    }

    // compute average
    adc_reading /= NO_OF_SAMPLES;

    {
        // Read ADC
        float voltage_mv = (float)hal.adc1->raw_to_voltage(adc_reading);

        // Convert adc_reading to cell voltage in mV
        cell_voltage_v =
            _easyResDiv(BATMAN_BATT_READ_RES, BATMAN_BATT_OTH_RES, voltage_mv) /
            1000.;
        cell_voltage_v /= BATMAN_BATT_NB_CELLS;

        LOGD(LTAG, "Cell: %f V\tRAW voltage: %fmV", cell_voltage_v, voltage_mv);
    }

    // SOC calculation with respect to the battery state

    if (m_bq_state == BQState::BQIdle) {
        // idling
        perc_batt = _percentageBatt(cell_voltage_v, 0., BATMAN_BATT_INT_RES_MO);
    } else if (m_bq_state == BQState::BQCharging) {
        // charging is in progress
        perc_batt = _percentageBatt(cell_voltage_v, BATMAN_BATT_AVG_CURRENT,
                                    BATMAN_BATT_INT_RES_MO);
    } else if (m_bq_state == BQState::BQCharged) {
        // charging is over
        perc_batt = 100;
    }

    if (perc_batt >= 0.) {
        m_batt_soc = (uint8_t)MIN(perc_batt, 100.);
#ifdef SHOW_INFO
        LOGI(LTAG, "New SOC: %u", m_batt_soc);
#endif
    } else {
        m_batt_soc = 0;

        LOGE(LTAG, "SOC error (soc= %d)", (int)perc_batt);
    }

    return m_batt_soc;
}

BQState BatteryMan::determine_bq_state(void) {
    //  STAT1
    //  Open-drain charge status output to indicate various charger operation.
    // Connect to the cathode of LED
    //  with 10 kΩ to the pullup rail. LOW or LED light up indicates charge in
    // progress. Otherwise stays HI or
    //  LED stays off.
    //
    //  STAT2
    //  Open-drain charge status output to indicate various charger operation.
    // Connect to the cathode of LED
    //  with 10 kΩ to the pullup rail. LOW or LED light up indicates charge is
    // complete. Otherwise, stays HI or
    //  LED stays off.

    //  When any fault condition occurs, both STAT1 and STAT2 are HI, or both
    //  LEDs are off.

    //                                                 STAT#    1   2
    //  Charge in progress                                      LO  HI
    //  Charge complete                                         HI  LO
    //  Charge suspend, overvoltage, sleep mode, battery absent HI  HI

    ePinState stat1_state = hal.gpio->get(STAT1_PIN);
    ePinState stat2_state = hal.gpio->get(STAT2_PIN);
 
    if (stat1_state && stat2_state) {
        // idling
        m_bq_state = BQState::BQIdle;
        LOGD(LTAG, "BQ Idle");
    } else if (stat1_state) {
        // charging is over
        m_bq_state = BQState::BQCharged;
        LOGD(LTAG, "BQ Charged");
    } else if (stat2_state) {
        // charging is in progress
        m_bq_state = BQState::BQCharging;
        LOGD(LTAG, "BQ Charging");
    } else {
        // not possible, report error
        m_bq_state = BQState::BQError;
        LOGE(LTAG, "BQ undefined state");
    }
 
    return m_bq_state;
}

void BatteryMan::update_pic_soc(void) {
    LOGD(LTAG, "Sending PIC info...");

    pic.sendBatteryInfo(this->m_batt_soc, this->m_bq_state);
    blue.updateAdv(this->m_batt_soc, this->m_bq_state == BQState::BQCharging);
}

/** @brief Calculates the MODE (most frequent number) of a ring buffer
 *
 * Logic
 * - buffer is appended in the 1st position
 * - sort the buffer to compute the MODE
 * - right rotation of the buffer
 *
 *@return the battery SOC -> PIC
 */
uint8_t BatteryMan::mode_of_rbuffer(void) {
    soc_buffer[0] = m_batt_soc;

    uint8_t m_batt_soc_mode = _mode_of_array(soc_buffer);

    std::rotate(soc_buffer.begin(), soc_buffer.begin() + soc_buffer.size() - 1,
                soc_buffer.end());

    // check if buffer is full
    if (!(std::count(soc_buffer.begin(), soc_buffer.end(), 0)))
        m_batt_soc = m_batt_soc_mode;
#ifdef SHOW_INFO
    LOGI(LTAG, "MODE SOC: %d", m_batt_soc);
#endif
    return m_batt_soc;
}


bool BatteryMan::is_PSU_plugged(void){

    if ((m_bq_state == BQState::BQCharged) || (m_bq_state == BQState::BQCharging)) {
        LOGI(LTAG, "PSU plugged");

        return true;
    }

    else {
        LOGI(LTAG, "PSU not plugged");
        return false;
    }

} 
