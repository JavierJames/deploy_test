#include "BluemanSat.hpp"

#include <assert.h>

#include "globals.hpp"
static PicmanInterface& pic = getPicman();

#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"

// RAHUL
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "bt_app_av.h"
#include "bt_app_core.h"
#include "bt_whitelist.h"

#include "a2dp/bt_app_av.h"
#include "a2dp/bt_app_core.h"

#include "ble/ble_adv_conf.hpp"
#include "ble/ble_central.hpp"
#include "ble/ble_gap.hpp"
#include "ble/ble_gatt.hpp"
#include "ble/ble_peripheral.hpp"
#include "ble/profiles/ble_nus_cmd.hpp"

static const char BT_TAG[] = "BluemanSat";

static SysInfo& sys = getSysInfo();

static char bt_name[20];

esp_err_t err;

BluemanSat::BluemanSat() : parser(pic_ota, esp_ota), stream(parser) {
    Kien::ble_adv_altbeacon_default(this->beacon);
    Kien::ble_adv_scanres_default(this->scanres);
}

void BluemanSat::init_dm() {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
        ESP_LOGE(BT_TAG, "BT controller init failed");
        return;
    }

    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
        ESP_LOGE(BT_TAG, "BT controller en failed");
        return;
    }

    if (esp_bluedroid_init() != ESP_OK) {
        ESP_LOGE(BT_TAG, "BT bluedroid init failed");
        return;
    }

    if (esp_bluedroid_enable() != ESP_OK) {
        ESP_LOGE(BT_TAG, "BT bluedroid en failed");
        return;
    }

    std::array<uint8_t, 3> id = {esp_bt_dev_get_address()[3],
                                 esp_bt_dev_get_address()[4],
                                 esp_bt_dev_get_address()[5]};
    sys.setSpkId(id);

    // Configure Bluetooth for audio
    bt_app_task_start_up(this->audioLinkEvt.handle);

    // Configure BLE
    std::array<char, FW_VER_LEN> pic;
    sys.getPicFwVersion(pic);
    std::array<char, FW_VER_LEN> esp;
    sys.getEspFwVersion(esp);
    // TODO: This prints an @ at the end of the version number
    ESP_LOGI(BT_TAG, "Setting advertised FW: ESP v%s, PIC v%s", esp.data(), pic.data());
    Kien::ble_adv_config_scanres_ver(scanres, pic, esp);
    Kien::ble_adv_config_scanres_name(scanres, true, id);

    memset(bt_name, 0, sizeof(bt_name));
    memcpy(bt_name, scanres.name_raw, scanres.name_len - 1);
    if (esp_bt_dev_set_device_name(bt_name) != ESP_OK) {
        ESP_LOGE(BT_TAG, "Setting BT name failed");
        return;
    }

    Kien::ble_gatt_init();
    Kien::ble_gap_init();
    Kien::ble_gap_register_peripheral(Kien::ble_peripheral_evt_handler);
    Kien::ble_gap_register_central(Kien::ble_central_evt_handler);
    Kien::ble_peripheral_set_advertisement(reinterpret_cast<uint8_t*>(&beacon),
                                           sizeof(Kien::altbeacon_t));
    Kien::ble_peripheral_set_scan_response(reinterpret_cast<uint8_t*>(&scanres),
                                           sizeof(Kien::scan_response_t));

    this->stream.create("ble_stream", 4096, 25);
    this->create("blueman", 2048, 20);
}

bool BluemanSat::getFormattedMAC(char* mac) {
    assert(mac);

    if (esp_bt_dev_get_address() == NULL) {
        return false;
    } else {
        snprintf(mac, sizeof(ESP_BD_ADDR_STR), ESP_BD_ADDR_STR,
                 ESP_BD_ADDR_HEX(esp_bt_dev_get_address()));
        ESP_LOGI(BT_TAG, "MAC: %s", mac);

        return true;
    }
}

const char* BluemanSat::getName() {
    ESP_LOGI(BT_TAG, "Name: %s", bt_name);
    return bt_name;
}

void BluemanSat::startBtPairing() {
    bt_app_work_dispatch(bt_av_hdl_stack_evt,
                         BT_APP_EVT_CONNECTABLE_DISCOVERABLE, NULL, 0, NULL);
}

void BluemanSat::stopBtPairing() {
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_CONNECTABLE, NULL, 0,
                         NULL);
}

void BluemanSat::disconnectAudioLink() {
    ESP_LOGI(BT_TAG, "Disconnect A2DP link");
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_DISCONNECT, NULL, 0,
                         NULL);
}

void BluemanSat::enableAudio() {
    ESP_LOGI(BT_TAG, "Enable A2DP stack");
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0,
                         NULL);
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_CONNECTABLE, NULL, 0,
                         NULL);
}

void BluemanSat::disableAudio() {
    ESP_LOGI(BT_TAG, "Disable A2DP stack");
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_DOWN, NULL, 0,
                         NULL);
}

void BluemanSat::enableBeacon() {
    ESP_LOGI(BT_TAG, "Enable BLE beacon");
    Kien::ble_peripheral_start_adv();
}

void BluemanSat::disableBeacon() {
    ESP_LOGI(BT_TAG, "Stop BLE beacon adv");
    Kien::ble_peripheral_stop_adv();
}

void BluemanSat::startBleScan() {
    // Scan starts if params are properly set
    Kien::ble_central_set_scan_params();
}

void BluemanSat::stopBleScan() {
    Kien::ble_central_stop_scan();
}

void BluemanSat::updateAdv(const std::array<char, FW_VER_LEN>& pic,
                           const std::array<char, FW_VER_LEN>& nrf) {
    if (this->adv_mtx.lock()) {
        ESP_LOGI(BT_TAG, "Updating FW info BLE adv");
        Kien::ble_adv_config_scanres_ver(this->scanres, pic, nrf);
        Kien::ble_peripheral_set_advertisement((uint8_t*)&this->beacon,
                                               sizeof(Kien::altbeacon_t));
        Kien::ble_peripheral_set_scan_response((uint8_t*)&this->scanres,
                                               sizeof(Kien::scan_response_t));
        this->adv_mtx.unlock();
    }
}

void BluemanSat::updateAdv(uint8_t battery, bool charging) {
    if (this->adv_mtx.lock()) {
#ifdef SHOW_INFO
        ESP_LOGI(BT_TAG, "Updating batt in BLE adv");
#endif
        Kien::ble_adv_config_altbeacon_battery(this->beacon, battery, charging);
        Kien::ble_peripheral_set_advertisement((uint8_t*)&this->beacon,
                                               sizeof(Kien::altbeacon_t));
        this->adv_mtx.unlock();
    }
}

void BluemanSat::sendGroupInfo(GroupInfo& group) {
    int id = group.getId();
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_CMD_SPACE_INFO;
    pkt.data[1] = id & 0xFF;
    pkt.data[2] = (id >> 8) & 0xFF;
    pkt.data[3] = (id >> 16) & 0xFF;
    pkt.data[4] = group.getNumMembers();
    pkt.len = 5;
    this->stream.txQueue.add(pkt);

    auto& members = group.getMembers();
    for (int i = 0; i < group.getNumMembers(); i++) {
        this->sendSpkInfo(members.at(i));
    }
}

void BluemanSat::sendGroupEvent() {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_CMD_SPACE_EVT;
    pkt.data[1] = 0xFF;
    pkt.data[2] = 0xFF;
    pkt.data[3] = 0xFF;
    pkt.len = 4;
    this->stream.txQueue.add(pkt);
}

void BluemanSat::sendSpkInfo(GroupMemberInfo& spk) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_CMD_SPACE_SPK;
    // TODO: add group info (bytes 1-3), space reserved
    std::copy(spk.id.begin(), spk.id.end(), pkt.data + 4);
    std::copy(spk.mac.begin(), spk.mac.end(), pkt.data + 4 + spk.id.size());
    pkt.len = 4 + spk.id.size() + spk.mac.size();
    this->stream.txQueue.add(pkt);
}

void BluemanSat::sendMasterVolume(uint8_t vol) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_VOL_MASTER;
    pkt.data[1] = vol;
    pkt.len = 2;
    this->stream.txQueue.add(pkt);
}

void BluemanSat::sendIndivVol(uint8_t vol, const std::array<uint8_t, 3>& id) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_VOL_INDIVIDUAL;
    pkt.data[1] = vol;
    std::copy(id.begin(), id.end(), pkt.data + 2);
    pkt.len = 5;
    this->stream.txQueue.add(pkt);
}

void BluemanSat::sendAudioConfig(const std::array<uint8_t, 3>& conf,
                                 const std::array<uint8_t, 3>& id) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_AUDIO_CONF;
    std::copy(conf.begin(), conf.end(), pkt.data + 1);
    std::copy(id.begin(), id.end(), pkt.data + 4);
    pkt.len = 7;
    this->stream.txQueue.add(pkt);
}

void BluemanSat::sendEqualizer(uint8_t eq) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_EQUALIZER;
    pkt.data[1] = eq;
    pkt.len = 2;
    this->stream.txQueue.add(pkt);
}

void BluemanSat::sendTouchRingMode(uint8_t mode) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_TOUCH_CTRL_MODE;
    pkt.data[1] = mode;
    pkt.len = 2;
    this->stream.txQueue.add(pkt);
}

void BluemanSat::factoryReset() {
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_DISCONNECT, NULL, 0,
                        NULL);

    // Wait for the disconnection to clear the whitelist (it won't work
    // otherwise)
    this->delay(5000);
    bt_whitelist_clear();
}

void BluemanSat::sendDWAMStatus(uint8_t status){
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_DWAM_STATUS;
    pkt.data[1] = status;
    pkt.len = 2;
    this->stream.txQueue.add(pkt);
}

void BluemanSat::send(nus_pkt_t& pkt) {
    this->stream.txQueue.add(pkt);
}

void BluemanSat::task() {
    for (;;) {
        this->audioLinkEvt.take();
        switch (bt_av_get_conn_status()) {

            case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
                pic.sendAudioLinkStatus(BTLinkStatus::DISCONNECTED);
                printf("DISCONNECTED\n");
                break;

            case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
                pic.sendAudioLinkStatus(BTLinkStatus::DISCONNECTING);
                printf("DISCONNECTING\n");
                break;

            case ESP_A2D_CONNECTION_STATE_CONNECTED:
                pic.sendAudioLinkStatus(BTLinkStatus::CONNECTED);
                printf("CONNECTED\n");
                break;

            case ESP_A2D_CONNECTION_STATE_CONNECTING:
                pic.sendAudioLinkStatus(BTLinkStatus::CONNECTING);
                printf("CONNECTING\n");
                break;

            default:
                ESP_LOGW(BT_TAG, "Unknown link event");
                break;
        }
        this->delay(100);
    }
    remove();
}

// void BluemanSat::delay(uint32_t delay_ms) {
//
// }

void BluemanSat::sendSerialNumber(uint8_t* serial_number) {
    if(serial_number == NULL) {
        ESP_LOGE(BT_TAG, "Failed to send serial number (NULL)");
        return;
    }

    nus_pkt_t pkt;

    pkt.data[0] = BLE_NUS_SERIAL_NUM;
    for(uint8_t i = 0; i < SERIAL_NUM_LEN; i++)
        pkt.data[i + 1] = serial_number[i];
    pkt.len = SERIAL_NUM_LEN + 1;

    this->stream.txQueue.add(pkt);
}
