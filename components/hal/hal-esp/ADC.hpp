#pragma once

#include <stdint.h>
#include "ADC_HAL.hpp"
#include "esp_adc_cal.h"

typedef struct sADCConfiguration {
    adc_atten_t atten;
    adc_unit_t adc_num;
    adc_bits_width_t bit_width;
    uint32_t vref;
    esp_adc_cal_characteristics_t adc_char;
} sADCConfiguration;

typedef struct sADC1Configuration {
    sADCConfiguration adc_gen_config;
    adc1_channel_t channel;
} sADC1Configuration;

/**
 * ADC driver for ESP32.
 */
class ADC_ESP : virtual public ADC_HAL {
   public:
    ADC_ESP(){};

   protected:
    uint32_t raw_to_voltage(uint32_t adc_reading, sADCConfiguration* config_);
    void characterize(sADCConfiguration* config_);
};

/**
 * ADC1 driver for ESP32.
 */
class ADC1_ESP : public ADC1_HAL, protected ADC_ESP {
   public:
    ADC1_ESP() : m_adc_config{} {};

    void begin(void) override;
    int get_raw(void) override;
    uint32_t raw_to_voltage(uint32_t adc_reading) override;

   private:
    sADC1Configuration m_adc_config;
    void config(void);
};
