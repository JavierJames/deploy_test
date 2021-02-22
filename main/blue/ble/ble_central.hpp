#pragma once

#include "esp_gap_ble_api.h"

#include "System_HAL.hpp"

namespace Kien {

/**
 * BLE peripheral event handler. This is usually called from the GAP
 * event callback (if registered).
 */
void ble_central_evt_handler(esp_gap_ble_cb_event_t event,
                             esp_ble_gap_cb_param_t* param);

/**
 * Starts scanning for BLE advertisement. This function should be called
 * after `ble_central_set_scan_params`.
 *
 * @param duration in seconds. If 0, scan permanently.
 * @returns Result::Ok when success, other value means something went wrong.
 */
Result ble_central_start_scan(int duration);

/**
 * Stops scanning for BLE advertisement.
 *
 * @returns Result::Ok when success, other value means something went wrong.
 */
Result ble_central_stop_scan();

/**
 * Configure the scan parameters.
 *
 * @returns Result::Ok when success, other value means something went wrong.
 */
Result ble_central_set_scan_params();

}  // namespace Kien
