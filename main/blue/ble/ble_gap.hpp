#pragma once

#include "esp_gap_ble_api.h"

#include "System_HAL.hpp"

namespace Kien {

/**
 * Configures the given data in the scan response packet.
 *
 * @returns Result::Ok when success, other value means something went wrong.
 */
Result ble_gap_init();

/**
 * Register a callback to handle the peripheral events.
 */
void ble_gap_register_peripheral(esp_gap_ble_cb_t callback);

/**
 * Register a callback to handle the central events.
 */
void ble_gap_register_central(esp_gap_ble_cb_t callback);

}  // namespace Kien
