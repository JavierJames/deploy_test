#include "Picman.hpp"

#include <cassert>
#include "comms/NMEAPkt.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <assert.h>

#include "globals.hpp"

#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"

#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "bt_app_av.h"
#include "bt_app_core.h"

#include "a2dp/bt_app_av.h"
#include "a2dp/bt_app_core.h"

#include "ble/ble_adv_conf.hpp"
#include "ble/ble_central.hpp"
#include "ble/ble_gap.hpp"
#include "ble/ble_gatt.hpp"
#include "ble/ble_peripheral.hpp"
#include "ble/profiles/ble_nus_cmd.hpp"
#include "spp/btc_spp_cmd.hpp"

#include "system/SysInfo.hpp"
static SysInfo& sys = getSysInfo();

static const char LTAG[] = "Picman";

static BluemanInterface& blue = getBlueman();
SemaphoreHandle_t mut_group = nullptr;

static HAL& hal = getHAL();


// static inline void delay(uint32_t delay_ms){};

void _mtx_group_unlock()
{
    static BaseType_t prior = pdFALSE;
    xSemaphoreGiveFromISR(mut_group, &prior);
}

Picman::Picman() : parser(newMode), stream(parser) {
    mut_group = xSemaphoreCreateBinary();
    this->parser.unlock_mut_group = _mtx_group_unlock;
    _mtx_group_unlock();
}

void Picman::start() {
    this->stream.create("pic_stream", 4096, 21);
    LOGD(LTAG, "PIC RX/TX task started");
}

void Picman::restart() {
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_RESET, nullptr);
    this->send(newpkt);
}

void Picman::requestGroupInfo() {
    if(xSemaphoreTake(mut_group, 10000) == pdTRUE)
    {
        this->parser.group.clear();
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_KN_SESS_SPK_REQ,
                       nullptr);
        this->send(newpkt);
    }else{
        LOGE(LTAG, "mut_group take failed");
    }
}

void Picman::requestIdentifyAction(uint8_t* id) {
    assert(id);

    char v_str[7];
    int res =
        snprintf(v_str, sizeof(v_str), "%02x%02x%02x", id[0], id[1], id[2]);

    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_IDENTIFY_SPEAKER, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in factoryReset");
    }
}

void Picman::startBootloader(bool start) {
    if (start) {
        this->bootloader = true;
        // TODO Flush buffer instead of clean it
        this->stream.txqueue.reset();
    } else {
        this->bootloader = false;
    }
}

void Picman::sendBatteryInfo(uint8_t bat, BQState state) {
    char b_info[20];
    memset(b_info, 0, sizeof(b_info));
#ifdef SHOW_INFO
    LOGI(LTAG, "BAT INFO: %u / %u", (uint8_t)bat, (uint8_t)state);
#endif
    // fixed hex format in 2 digits
    int res =
        snprintf(b_info, sizeof(b_info), "%02x,%02x", bat, (uint8_t)state);

    if (res > 0 && res < sizeof(b_info)) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC, b_info);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in getFormattedBatteryInfo");
    }
}

void Picman::sendUTADatasetSample(char* pdata) {
    assert(pdata);

    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_UTA_DATASET_BROADCAST,
                   pdata);
    this->send(newpkt);
}

void Picman::send(NMEAPkt& pkt) {
    if (this->bootloader == false) {
        this->stream.txqueue.add(pkt);
    } else {
        // Do NOT send NMEA packets while PIC bootloader running because it will
        // corrupt the image
        LOGD(LTAG, "PIC update is in prograss");
    }
}

void Picman::send(OTAPkt& pkt) {
    this->stream.txqueue.add(pkt);

}

void Picman::send(spp_pkt_t& pkt) {

    //Send directly to UART
    hal.uartPIC->write(pkt.data, pkt.len);
}

void Picman::sendAudioLinkStatus(BTLinkStatus status) {
    char l_info[3];

    int res =
        snprintf(l_info, sizeof(l_info), "%02x", static_cast<uint8_t>(status));

    if (res > 0 && res < sizeof(l_info)) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_BT_LINK_STATUS, l_info);
        sleep(1); // 14 Apr: Added the 1 second delay to investigate if this helps communication with the PIC
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in sendAudioLinkStatus");
    }
}

void Picman::setVolumeMaster(uint8_t vol) {
    char v_str[3];
    int res = snprintf(v_str, sizeof(v_str), "%02x", vol);
    if (res == 2) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_VOL_MASTER_SET, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in setVolumeMaster");
    }
}

void Picman::requestVolumeMaster() {
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_VOL_MASTER_REQ, nullptr);
    this->send(newpkt);
}

void Picman::setVolume(uint8_t* id, uint8_t vol) {
    assert(id);

    char v_str[10];
    int res = snprintf(v_str, sizeof(v_str), "%02x%02x%02x,%02x", id[0], id[1],
                       id[2], vol);
    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_VOL_INDIV_SET, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in setVolume");
    }
}

void Picman::requestVolume(uint8_t* id) {
    assert(id);

    char v_str[7];
    int res =
        snprintf(v_str, sizeof(v_str), "%02x%02x%02x", id[0], id[1], id[2]);
    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_VOL_INDIV_REQ, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in requestVolume");
    }
}

void Picman::factoryReset(uint8_t* id) {
    assert(id);

    char v_str[7];
    int res =
        snprintf(v_str, sizeof(v_str), "%02x%02x%02x", id[0], id[1], id[2]);

    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_FACTORY_RESET_REQ,
                       v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in factoryReset");
    }
}

void Picman::requestAudioConfig(uint8_t* id) {
    assert(id);

    char v_str[7];
    int res =
        snprintf(v_str, sizeof(v_str), "%02x%02x%02x", id[0], id[1], id[2]);
    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_AUDIO_CONFIG_REQ, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in requestAudioConfig");
    }
}

void Picman::setAudioConfig(uint8_t* id, uint8_t* config) {
    assert(id);
    assert(config);

    char v_str[14];
    int res = snprintf(v_str, sizeof(v_str), "%02x%02x%02x,%02x%02x%02x",
                       config[0], config[1], config[2], id[0], id[1], id[2]);
    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_AUDIO_CONFIG_SET, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in setAudioConfig");
    }
}

void Picman::requestEqualizer() {
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMMS_AUDIO_EQ_REQ, nullptr);
    this->send(newpkt);
}

void Picman::setEqualizer(uint8_t eq) {
    char v_str[3];
    int res = snprintf(v_str, sizeof(v_str), "%02x", eq);
    if (res == 2) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMMS_AUDIO_EQ_SET, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in setEqualizer");
    }
}

void Picman::requestTouchRingMode() {
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_TRING_MODE_REQ, nullptr);
    this->send(newpkt);
}

void Picman::enableDWAMasCU() {
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMMS_ENABLE_DWAM_CU, nullptr);
    this->send(newpkt);
}

void Picman::acceptNewFollower() {
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMMS_ACCEPT_FOLLOWER, nullptr);
    this->send(newpkt);
}

void Picman::setTouchRingMode(uint8_t mode) {
    char v_str[3];
    int res = snprintf(v_str, sizeof(v_str), "%02x", mode);
    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_TRING_MODE_SET, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in setTouchRingMode");
    }
}

void Picman::requestSpeakerMode() {
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_SPEAKER_MODE_REQ, nullptr);
    this->send(newpkt);
}

// MADE UP AND DOESNT EXIST!
/*
void Picman::setSpeakerMode() {
    char v_str[3];
    int res = snprintf(v_str, sizeof(v_str), "%02x", mode);
    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_SPEAKER_MODE_SET, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in setSpeakerMode");
    }
}
*/

void Picman::disconnectFollower(uint8_t* id) {
    assert(id);

    char v_str[7];
    int res =
        snprintf(v_str, sizeof(v_str), "%02x%02x%02x", id[0], id[1], id[2]);

    if ((res > 0) && (res < sizeof(v_str))) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_KN_SESS_SPK_DEL, v_str);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in disconnectFollower");
    }
}

void Picman::sendAccelStatus(uint8_t status) {
    char acc_info[3];

    int res = snprintf(acc_info, sizeof(acc_info), "%02x", status);
    if (res > 0 && res < sizeof(acc_info)) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_ACCEL_READ, acc_info);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in sendAccelStatus");
    }
}

void Picman::sendOTAespStatus(FwUpdtStatus status) {
    char ota_info[3];
    LOGI(LTAG, "SENDING OTA FW STATUS TO PIC : %d", static_cast<uint8_t>(status));

    int res =
        snprintf(ota_info, sizeof(ota_info), "%02x", static_cast<uint8_t>(status));

    if (res > 0 && res < sizeof(ota_info)) {
        NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_OTA_ESP_STATUS, ota_info);
        this->send(newpkt);
    } else {
        LOGE(LTAG, "snprintf error in sendOTAespStatus");
    }
}

void Picman::sendSleepMessage()
{
    LOGI(LTAG, "SENDING SLEEP MSG TO PIC"); 
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_SLEEP, nullptr);
    this->send(newpkt);
}

void Picman::sendStandbyMessage()
{
    LOGI(LTAG, "SENDING STANDBY MSG TO PIC"); 
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_STANDBY, nullptr);

    this->send(newpkt);
}


void Picman::sendPSUPluggedTimeout()
{
    LOGI(LTAG, "SENDING POWER OFF MSG TO PIC"); 
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_PSU_PLUGGED_TIMEOUT, nullptr);
    this->send(newpkt);
}

 
void Picman::requestSerialNumber()
{
    LOGI(LTAG, "Requesting Serial number"); 
    NMEAPkt newpkt(NMEAPkt::NMEAPktPages::PICCOMMS_SERIAL_NUM_REQ, nullptr);
    this->send(newpkt);
}


