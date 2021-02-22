#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_spp_api.h"
#include "btc_spp_cmd.hpp"


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*parser_callback)(spp_pkt_t* pkt);
/**
 * Handler function to process the GATT events. A pointer to this function
 * shall be passed when creating the table.
 */
void btc_spp_evt_handlr(esp_spp_cb_event_t event,
					esp_spp_cb_param_t *param);



bool btc_spp_init(void (*parser)(spp_pkt_t* pkt));

void btc_spp_init_crc();

uint16_t btc_spp_crc();

bool btc_spp_deinit();

void btc_spp_init_ringBuff(xQueueHandle rx_cmd_queue);

void btc_spp_send(uint8_t* buff, size_t len);

#ifdef __cplusplus
}
#endif
