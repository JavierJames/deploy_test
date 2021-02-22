#pragma once

#include <stdint.h>

#define NUS_MTU_MAX 200
// The reason why this is an struct is to be able to use sizeof(nus_pkt_t) when
// creating a Queue using C++ templates
typedef struct nus_pkt_t {
    uint8_t data[NUS_MTU_MAX];
    uint8_t len;
} nus_pkt_t;

#define BLE_NUS_CMD_ESP_OTA_BEGIN 0x10
#define BLE_NUS_CMD_ESP_OTA_BEGIN_RES 0x11
#define BLE_NUS_CMD_PIC_OTA_BEGIN 0x12
#define BLE_NUS_CMD_PIC_OTA_BEGIN_RES 0x13
#define BLE_NUS_CMD_OTA_CHUNK 0x14
#define BLE_NUS_CMD_OTA_CHUNK_ACK 0x15
#define BLE_NUS_CMD_OTA_RES 0x16
#define BLE_NUS_CMD_OTA_ABORT 0x17

#define BLE_NUS_CMD_SPK_MODE_REQ 0x6D
#define BLE_NUS_CMD_SPK_MODE 0x6E

#define BLE_NUS_CMD_SPACE_INFO_REQ 0x50
#define BLE_NUS_CMD_SPACE_INFO 0X51
#define BLE_NUS_CMD_SPACE_EVT 0X52
#define BLE_NUS_CMD_SPACE_SPK 0X53

#define BLE_NUS_IDENTIFY_SPK_REQ 0x69
#define BLE_NUS_CMD_FACTORY_RESET_REQ 0x72

#define BLE_NUS_EQUALIZER_SET 0x31
#define BLE_NUS_EQUALIZER_REQ 0x32
#define BLE_NUS_EQUALIZER 0x33

#define BLE_NUS_POSITION_SET 0x34  // Deprecated
#define BLE_NUS_POSITION_REQ 0x35  // Deprecated
#define BLE_NUS_POSITION 0x36      // Deprecated

#define BLE_NUS_VOL_MASTER_SET 0x40
#define BLE_NUS_VOL_MASTER_REQ 0x41
#define BLE_NUS_VOL_MASTER 0x42

#define BLE_NUS_VOL_INDIVIDUAL_SET 0x46
#define BLE_NUS_VOL_INDIVIDUAL_REQ 0x47
#define BLE_NUS_VOL_INDIVIDUAL 0x48

#define BLE_NUS_SERIAL_NUM 0x5E

#define BLE_NUS_CHN_MODE_SET 0X60  // Deprecated
#define BLE_NUS_CHN_MODE_REQ 0X61  // Deprecated
#define BLE_NUS_CHN_MODE 0X62      // Deprecated

#define BLE_NUS_TOUCH_CTRL_MODE_SET 0X63
#define BLE_NUS_TOUCH_CTRL_MODE_REQ 0X64
#define BLE_NUS_TOUCH_CTRL_MODE 0X65

#define BLE_NUS_AUDIO_CONF_SET 0X66
#define BLE_NUS_AUDIO_CONF_REQ 0X67
#define BLE_NUS_AUDIO_CONF 0X68

#define BLE_NUS_FOLLOWER_REMOVE 0x71

#define BLE_NUS_ACCEPT_FOLLOWER 0x74
#define BLE_NUS_ENABLE_DWAM 0x75
#define BLE_NUS_DWAM_STATUS 0x76


