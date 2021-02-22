#include "BleParser.hpp"

#include <assert.h>
#include "globals.hpp"
static PicmanInterface& pic = getPicman();
static BluemanInterface& blue = getBlueman();

#include "ble/profiles/ble_nus_cmd.hpp"

static const char BT_TAG[] = "BleParser";

#include "system/SysInfo.hpp"
static SysInfo& sys = getSysInfo();

static void _handle_pkt_err(uint8_t id) {
    LOGE(BT_TAG, "Wrong pkt 0x%02X", id);
}

static void _log_id(uint8_t* id) {
    LOGI(BT_TAG, " - SPK: 0x%02X%02X%02X", id[2], id[1], id[0]);
}

BleParser::BleParser(PICUpdate& _pic_ota, ESPUpdate& _esp_ota)
    : pic_ota(_pic_ota), esp_ota(_esp_ota), active_ota(nullptr) {}

void BleParser::parsePkt(nus_pkt_t* pkt, Queue<nus_pkt_t>& txqueue) {
    assert(pkt);

    switch (pkt->data[0]) {
        case BLE_NUS_IDENTIFY_SPK_REQ:
            LOGI(BT_TAG, "Identify yourself!");
            if (pkt->len == 4) {
                _log_id(pkt->data + 1);
                pic.requestIdentifyAction(pkt->data + 1);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_CMD_PIC_OTA_BEGIN:
            if (pkt->len == sizeof(ota_init_pkt_t) + 1) {
                pic_ota.start(
                    *reinterpret_cast<ota_init_pkt_t*>(&pkt->data[1]));
                active_ota = &pic_ota;
            } else {
                LOGE(BT_TAG, "Invalid OTA init pkt (%i)", pkt->len);
            }
            break;

        case BLE_NUS_CMD_ESP_OTA_BEGIN:
            if (pkt->len == sizeof(ota_init_pkt_t) + 1) {
                esp_ota.start(
                    *reinterpret_cast<ota_init_pkt_t*>(&pkt->data[1]));
                active_ota = &esp_ota;
            } else {
                LOGE(BT_TAG, "Invalid OTA init pkt (%i)", pkt->len);
            }
            break;

        case BLE_NUS_CMD_OTA_CHUNK:
            if (active_ota != nullptr) {
                active_ota->processPkt(*pkt);
            } else {
                LOGE(BT_TAG, "Update has not started");
            }
            break;

        case BLE_NUS_CMD_OTA_ABORT:
            if (active_ota != nullptr) {
                active_ota->abort();
                active_ota = nullptr;
            } else {
                LOGW(BT_TAG, "Update has not started");
            }
            break;

        case BLE_NUS_CMD_SPK_MODE:
            LOGI(BT_TAG, "Requesting mode");
            SpkMode mode;
            if (sys.getCurrentSpkMode(&mode)) {
                // TODO: this is not what the protocol specifies. It
                // should send {0x6E, mode}. For some reason this is not
                // what the app expects.
                if (mode == SpkMode::LEADER) {
                    pkt->data[0] = '0';
                } else if (mode == SpkMode::FOLLOWER) {
                    pkt->data[0] = '1';
                } else {
                    break;
                }
                pkt->len = 1;
                txqueue.add(*pkt);
            }
            break;

        case BLE_NUS_CMD_FACTORY_RESET_REQ:
            LOGI(BT_TAG, "Factory reset");
            if (pkt->len == 4) {
                _log_id(pkt->data + 1);
                pic.factoryReset(pkt->data + 1);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_CMD_SPACE_INFO_REQ:
            LOGI(BT_TAG, "Requesting space info");
            pic.requestGroupInfo();
            break;

        case BLE_NUS_VOL_MASTER_SET:
            LOGI(BT_TAG, "Setting mvol");
            if (pkt->len == 2) {
                pic.setVolumeMaster(pkt->data[1]);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_VOL_MASTER_REQ:
            LOGI(BT_TAG, "Requesting mvol");
            pic.requestVolumeMaster();
            break;

        case BLE_NUS_VOL_INDIVIDUAL_SET:
            LOGI(BT_TAG, "Setting ivol");
            if (pkt->len == 5) {
                LOGI(BT_TAG, " - %i %%", pkt->data[1]);
                _log_id(pkt->data + 2);
                pic.setVolume(pkt->data + 2, pkt->data[1]);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_VOL_INDIVIDUAL_REQ:
            LOGI(BT_TAG, "Requesting ivol");
            if (pkt->len == 4) {
                _log_id(pkt->data + 1);
                pic.requestVolume(pkt->data + 1);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }

            break;

        case BLE_NUS_EQUALIZER_SET:
            LOGI(BT_TAG, "Setting equalizer");
            if (pkt->len == 2) {
                pic.setEqualizer(pkt->data[1]);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_EQUALIZER_REQ:
            LOGI(BT_TAG, "Requesting equalizer");
            pic.requestEqualizer();
            break;

        case BLE_NUS_AUDIO_CONF_SET:
            LOGI(BT_TAG, "Setting audio config");
            if (pkt->len == 7) {
                LOGI(BT_TAG, " - Conf: %02X:%02X:%02X", pkt->data[1],
                     pkt->data[2], pkt->data[3]);
                _log_id(pkt->data + 4);
                pic.setAudioConfig(pkt->data + 1, pkt->data + 4);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_AUDIO_CONF_REQ:
            LOGI(BT_TAG, "Requesting audio config");
            if (pkt->len == 4) {
                _log_id(pkt->data + 1);
                pic.requestAudioConfig(pkt->data + 1);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_TOUCH_CTRL_MODE_SET:
            LOGI(BT_TAG, "Setting touch mode control");
            if (pkt->len == 2) {
                pic.setTouchRingMode(pkt->data[1]);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_TOUCH_CTRL_MODE_REQ:
            LOGI(BT_TAG, "Requesting touch mode control");
            pic.requestTouchRingMode();
            break;

        case BLE_NUS_ENABLE_DWAM:
            LOGI(BT_TAG, "Enable DWAM as CU");
            pic.enableDWAMasCU();
            break;

        case BLE_NUS_ACCEPT_FOLLOWER:
            LOGI(BT_TAG, "Accept new follower");
            pic.acceptNewFollower();
            break;

        case BLE_NUS_FOLLOWER_REMOVE:
            LOGI(BT_TAG, "Remove follower from group");
            if (pkt->len == 4) {
                _log_id(pkt->data + 1);
                pic.disconnectFollower(pkt->data + 1);
            } else {
                _handle_pkt_err(pkt->data[0]);
            }
            break;

        case BLE_NUS_SERIAL_NUM: {
            LOGI(BT_TAG, "Request serial number");

            if(!sys.isSerialNumberLoaded())
                pic.requestSerialNumber();
            else {
                uint8_t* serial_num = sys.getSerialNumber();
                blue.sendSerialNumber(serial_num);
            }

            }
            break;

        default:
            LOGW(BT_TAG, "Unknown CMD 0x%02X", pkt->data[0]);
            break;
    }
}
