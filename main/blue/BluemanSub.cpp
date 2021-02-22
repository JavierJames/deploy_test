#include "BluemanSub.hpp"

#include <assert.h>

#include "globals.hpp"

#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"

#include "a2dp/bt_app_av.h"
#include "a2dp/bt_app_core.h"

#include "ble/ble_adv_conf.hpp"
#include "ble/ble_central.hpp"
#include "ble/ble_gap.hpp"
#include "ble/ble_gatt.hpp"
#include "ble/ble_peripheral.hpp"
#include "ble/profiles/ble_nus_cmd.hpp"

static const char BT_TAG[] = "BluemanSub";

static SysInfo& sys = getSysInfo();

static char bt_name[20];

BluemanSub::BluemanSub() : parser(pic_ota, esp_ota), stream(parser) {
    Kien::ble_adv_altbeacon_default(this->beacon);
    Kien::ble_adv_scanres_default(this->scanres);
}

void BluemanSub::init_lem() {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    // TODO Disable in Subwoofer's sdkconfig to reduce binary size
    bt_cfg.mode = ESP_BT_MODE_BLE;

    if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
        ESP_LOGE(BT_TAG, "BT controller init failed");
        return;
    }

    if (esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) {
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

    // Configure BLE
    std::array<char, FW_VER_LEN> pic;
    sys.getPicFwVersion(pic);
    std::array<char, FW_VER_LEN> esp;
    sys.getEspFwVersion(esp);
    Kien::ble_adv_config_scanres_ver(scanres, pic, esp);
    Kien::ble_adv_config_scanres_name(scanres, false, id);

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

    this->stream.create("ble_stream", 4096, 20);
}

bool BluemanSub::getFormattedMAC(char* mac) {
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

const char* BluemanSub::getName() {
    ESP_LOGI(BT_TAG, "Name: %s", bt_name);
    return bt_name;
}

void BluemanSub::startBtPairing() {
    ESP_LOGW(BT_TAG, "Classic BT not available");
}

void BluemanSub::stopBtPairing() {
    ESP_LOGW(BT_TAG, "Classic BT not available");
}

void BluemanSub::disconnectAudioLink() {
    ESP_LOGW(BT_TAG, "Classic BT not available");
}

void BluemanSub::enableAudio() {
    ESP_LOGW(BT_TAG, "Classic BT not available");
}

void BluemanSub::disableAudio() {
    ESP_LOGW(BT_TAG, "Classic BT not available");
}

void BluemanSub::enableBeacon() {
    ESP_LOGI(BT_TAG, "Enable BLE beacon");
    Kien::ble_peripheral_start_adv();
}

void BluemanSub::disableBeacon() {
    ESP_LOGI(BT_TAG, "Stop BLE beacon adv");
    Kien::ble_peripheral_stop_adv();
}

void BluemanSub::startBleScan() {
    // Scan starts if params are properly set
    Kien::ble_central_set_scan_params();
}

void BluemanSub::stopBleScan() {
    Kien::ble_central_stop_scan();
}

void BluemanSub::updateAdv(const std::array<char, FW_VER_LEN>& pic,
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

void BluemanSub::updateAdv(uint8_t battery, bool charging) {
    if (this->adv_mtx.lock()) {
        ESP_LOGI(BT_TAG, "Updating batt in BLE adv");
        Kien::ble_adv_config_altbeacon_battery(this->beacon, battery, charging);
        Kien::ble_peripheral_set_advertisement((uint8_t*)&this->beacon,
                                               sizeof(Kien::altbeacon_t));
        this->adv_mtx.unlock();
    }
}

void BluemanSub::sendGroupInfo(GroupInfo& group) {
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

void BluemanSub::sendGroupEvent() {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_CMD_SPACE_EVT;
    pkt.data[1] = 0xFF;
    pkt.data[2] = 0xFF;
    pkt.data[3] = 0xFF;
    pkt.len = 4;
    this->stream.txQueue.add(pkt);
}

void BluemanSub::sendSpkInfo(GroupMemberInfo& spk) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_CMD_SPACE_SPK;
    // TODO: add group info (bytes 1-3), space reserved
    std::copy(spk.id.begin(), spk.id.end(), pkt.data + 4);
    std::copy(spk.mac.begin(), spk.mac.end(), pkt.data + 4 + spk.id.size());
    pkt.len = 4 + spk.id.size() + spk.mac.size();
    this->stream.txQueue.add(pkt);
}

void BluemanSub::sendMasterVolume(uint8_t vol) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_VOL_MASTER;
    pkt.data[1] = vol;
    pkt.len = 2;
    this->stream.txQueue.add(pkt);
}

void BluemanSub::sendIndivVol(uint8_t vol, const std::array<uint8_t, 3>& id) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_VOL_INDIVIDUAL;
    pkt.data[1] = vol;
    std::copy(id.begin(), id.end(), pkt.data + 2);
    pkt.len = 5;
    this->stream.txQueue.add(pkt);
}

void BluemanSub::sendAudioConfig(const std::array<uint8_t, 3>& conf,
                                 const std::array<uint8_t, 3>& id) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_AUDIO_CONF;
    std::copy(conf.begin(), conf.end(), pkt.data + 1);
    std::copy(id.begin(), id.end(), pkt.data + 4);
    pkt.len = 7;
    this->stream.txQueue.add(pkt);
}

void BluemanSub::sendEqualizer(uint8_t eq) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_EQUALIZER;
    pkt.data[1] = eq;
    pkt.len = 2;
    this->stream.txQueue.add(pkt);
}

void BluemanSub::sendTouchRingMode(uint8_t mode) {
    nus_pkt_t pkt;
    pkt.data[0] = BLE_NUS_TOUCH_CTRL_MODE;
    pkt.data[1] = mode;
    pkt.len = 2;
    this->stream.txQueue.add(pkt);
}

void BluemanSub::factoryReset() {
    ESP_LOGW(BT_TAG, "Classic BT not available");
}

void BluemanSub::send(nus_pkt_t& pkt) {
    this->stream.txQueue.add(pkt);
}


void BluemanSub::sendDWAMStatus(uint8_t status){
    ESP_LOGE(BT_TAG, "Funciton not supported by SUB");
}

void BluemanSub::sendSerialNumber(uint8_t* serial_number) {
    nus_pkt_t pkt;

    pkt.data[0] = BLE_NUS_SERIAL_NUM;
    for(uint8_t i = 0; i < SERIAL_NUM_LEN; i++)
        pkt.data[i + 1] = serial_number[i];
    pkt.len = SERIAL_NUM_LEN + 1;

    this->stream.txQueue.add(pkt);
}
