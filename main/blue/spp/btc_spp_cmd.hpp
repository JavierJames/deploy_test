#pragma once

#include <stdint.h>

#include "../ble/profiles/ble_nus_cmd.hpp"

#define SPP_MAX_ALLOWED 1000 //maxium allowed bytes to ber parsed per received SPP packet
#define SPP_WRAPPER_ON 1  
#define SPP_WRAPPER_OFF 0  
#define SPP_WRAPPER_STATUS SPP_WRAPPER_ON // 0 is off, 1 is on



// The reason why this is an struct is to be able to use sizeof(spp_pkt_t) when
// creating a Queue using C++ templates
typedef struct spp_pkt_t {
    uint8_t data[SPP_MAX_ALLOWED];
    uint16_t len;
} spp_pkt_t;

#define BTC_SPP_CMD_ESP_OTA_BEGIN BLE_NUS_CMD_ESP_OTA_BEGIN
#define BTC_SPP_CMD_ESP_OTA_BEGIN_RES BLE_NUS_CMD_ESP_OTA_BEGIN_RES
#define BTC_SPP_CMD_PIC_OTA_BEGIN BLE_NUS_CMD_PIC_OTA_BEGIN
#define BTC_SPP_CMD_PIC_OTA_BEGIN_RES BLE_NUS_CMD_PIC_OTA_BEGIN_RES
#define BTC_SPP_CMD_OTA_CHUNK BLE_NUS_CMD_OTA_CHUNK
#define BTC_SPP_CMD_OTA_CHUNK_ACK BLE_NUS_CMD_OTA_CHUNK_ACK
#define BTC_SPP_CMD_OTA_RES BLE_NUS_CMD_OTA_RES
#define BTC_SPP_CMD_OTA_ABORT BLE_NUS_CMD_OTA_ABORT
