#pragma once

#include <cstdint>
#include "esp_gap_ble_api.h"

#include "System_HAL.hpp"

namespace Kien {

/**
 * BLE peripheral event handler. This is usually called from the GAP
 * event callback (if registered).
 */
void ble_peripheral_evt_handler(esp_gap_ble_cb_event_t event,
                                esp_ble_gap_cb_param_t* param);

/**
 * Starts broadcasting BLE advertising. This function should be called
 * after `ble_adv_config_data`.
 *
 * @returns Result::Ok when success, other value means something went wrong.
 */
Result ble_peripheral_start_adv();

/**
 * Initializes the advertising module by setting up the required callbacks
 * and configuring the default adv payload.
 *
 * @returns Result::Ok when success, other value means something went wrong.
 */
Result ble_peripheral_stop_adv();

/**
 * Configures the given data in the advertisement packet.
 *
 * @param data payload of the advertisement data.
 * @param len number of bytes of the scan response.
 * @returns Result::Ok when success, other value means something went wrong.
 */
Result ble_peripheral_set_advertisement(uint8_t* data, uint8_t len);

/**
 * Configures the given data in the scan response packet.
 *
 * @param data payload of the scan response.
 * @param len number of bytes of the scan response.
 * @returns Result::Ok when success, other value means something went wrong.
 */
Result ble_peripheral_set_scan_response(uint8_t* data, uint8_t len);

}  // namespace Kien
