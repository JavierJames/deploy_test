#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "ble_nus_cmd.hpp"
#include "esp_gatts_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handler function to process the GATT events. A pointer to this function
 * shall be passed when creating the table.
 */
void nus_evt_handlr(esp_gatts_cb_event_t event,
                    esp_gatt_if_t gatts_if,
                    esp_ble_gatts_cb_param_t* param);

void ble_nus_send(uint8_t* buff, size_t len);

void ble_nus_init(xQueueHandle rx_cmd_queue);

#ifdef __cplusplus
}
#endif
