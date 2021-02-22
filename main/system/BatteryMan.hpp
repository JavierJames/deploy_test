#pragma once

#include "HAL.hpp"

#define BATTERY_MANAGER_PERIOD_MS 1000 * 60
#define BATTERY_MANAGER_BQ_CHARGE_CURRENT 1.333

enum class BQState : uint8_t { BQIdle, BQCharging, BQCharged, BQError };

/**
 * Thread-safe class to access system info by the different tasks (methods use a
 * Mutex to access the resources).
 */
class BatteryMan : public Task {
   public:
    /**
     * Class constructor.
     */
    BatteryMan();

    /**
     * Starts the battery management task, which measures the SOC
     * and sends it to the PIC every BATTERY_MANAGER_PERIOD_MS milliseconds
     */
    void start(void);
    bool is_PSU_plugged(void);

    uint8_t measure_soc(void);
    void update_pic_soc(void);
    BQState determine_bq_state(void);
    uint8_t mode_of_rbuffer(void);

   private:
    uint8_t m_batt_soc;
    BQState m_bq_state;

    void task() override;

};
