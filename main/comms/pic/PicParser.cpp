#include "PicParser.hpp"

#include <assert.h>
#include "globals.hpp"

static BluemanInterface& blue = getBlueman();
static PicmanInterface& pic = getPicman();
static BTC_BluemanInterface& blueBT = getBTCBlueman();
static SysInfo& sys = getSysInfo();

static const char LTAG[] = "PicParser";
char mac[20];

#include <algorithm>  // std::copy

#include "a2dp/bt_app_core.h"
#include "HAL.hpp"


extern bool m_auto_pwr_off_enabled;

static HAL& hal = getHAL();

PicParser::PicParser(BinarySemaphore& _newMode) : newMode(_newMode){}

void PicParser::parsePkt(NMEAPkt& pkt, Queue<SerialPkt>* txqueue) {
    char* param;
    size_t param_len;
    NMEAPkt pktTx;

    switch (pkt.page) {
        case NMEAPkt::NMEAPktPages::PICCOMMS_INVALID_PAGE:
            LOGE(LTAG, "Page not valid");
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_FW_NAME:
            param_len = pkt.nextParam(&param);
            sys.setPicFwName(param, param_len);
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_FW_VERSION:
            param_len = pkt.nextParam(&param);
            if (param_len != FW_VER_LEN) {
                LOGE(LTAG, "FW version length incorrect (%i)", param_len);
            } else {
                const std::array<char, FW_VER_LEN> pic = {param[0], param[1],
                                                          param[2]};
                if (sys.setPicFwVersion(pic)) {
                    std::array<char, FW_VER_LEN> esp;
                    sys.getEspFwVersion(esp);
                    blue.updateAdv(pic, esp);
                }
            }
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_SPEAKER_MODE:
            param_len = pkt.nextParam(&param);
            if ((param_len == 0) || (param == NULL)) {
                LOGW(LTAG, "Param missing");
                return;
            }
            sys.setCurrentSpkMode(static_cast<SpkMode>(param[0]));
            newMode.give();
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_SYSTEM_STATUS_REQ:
            assert(txqueue);

            pktTx.encode(NMEAPkt::NMEAPktPages::PICCOMMS_SYSTEM_STATUS,
                         nullptr);
            txqueue->add(pktTx);
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_SERIAL_NUM: {
            param_len = pkt.nextParam(&param);

            uint8_t serial_num[SERIAL_NUM_LEN];

            if (param_len != sizeof(serial_num)) {
                LOGE(LTAG, "Unknown serial number len (%i)", param_len);
                return;
            }
            
            memcpy(serial_num, param, SERIAL_NUM_LEN);

            /* convert from ascii to int */
            for(uint8_t i = 0; i < SERIAL_NUM_LEN; i++)
                serial_num[i] -= '0';

            sys.setSerialNumber(serial_num);
            blue.sendSerialNumber(serial_num);
            }
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_BTMAC_ADDR_REQ:
            assert(txqueue);

            blue.getFormattedMAC(mac);
            pktTx.encode(NMEAPkt::NMEAPktPages::PICCOMMS_BTMAC_ADDR, mac);
            txqueue->add(pktTx);

            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_BTNAME_REQ:
            assert(txqueue);

            pktTx.encode(NMEAPkt::NMEAPktPages::PICCOMMS_BTNAME,
                         (char*)blue.getName());
            txqueue->add(pktTx);
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_BT_STATUS_SET:
            param_len = pkt.nextParam(&param);
            if (param_len != 2) {
                LOGE(LTAG, "Unknown BT status len (%i)", param_len);
                return;
            }

            /*
                -'pn' : Pairing oN(module discoverable)
                -'pf' : Pairing oFf(module not discoverable)
                -'cn' : Connectability oN(module connectable)
                -'cf' : Connectability oFf(module not connectable)
                -'bn' : Bluetooth oN
                -'bf' : Bluetooth oFf
            */
            if (!memcmp(param, "pn", 2)) {
                LOGI(LTAG, "Start BT pairing cmd received");
                blue.startBtPairing();
            } else if (!memcmp(param, "pf", 2)) {
                LOGI(LTAG, "Stop BT pairing cmd received");
                blue.stopBtPairing();
            } else if (!memcmp(param, "cn", 2)) {
                LOGI(LTAG, "Start BT connectability cmd received");
                // TODO blue.enableAudio();
            } else if (!memcmp(param, "cf", 2)) {
                LOGI(LTAG, "Stop BT connectability cmd received");
                // TODO blue.disableAudio();
            } else if (!memcmp(param, "bn", 2)) {
                // TODO
            } else if (!memcmp(param, "bf", 2)) {
                // TODO
            } else {
                LOGE(LTAG, "Unknown request (%c)", param[0]);
            }

            break;
            
        case NMEAPkt::NMEAPktPages:: PICCOMMS_BT_LINK_STATUS_REQ:
            LOGI(LTAG, "Link status request");
            //TODO: clean
            switch (bt_av_get_conn_status()) {

                case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
                    pic.sendAudioLinkStatus(BTLinkStatus::DISCONNECTED);
                    break;

                case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
                    pic.sendAudioLinkStatus(BTLinkStatus::DISCONNECTING);
                    break;

                case ESP_A2D_CONNECTION_STATE_CONNECTED:
                    pic.sendAudioLinkStatus(BTLinkStatus::CONNECTED);
                    break;

                case ESP_A2D_CONNECTION_STATE_CONNECTING:
                    pic.sendAudioLinkStatus(BTLinkStatus::CONNECTING);
                    break;

                default:
                    ESP_LOGW(LTAG, "Unknown link event");
                    break;
            }
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_OTA_PIC_STATUS:
            param_len = pkt.nextParam(&param);
            if (param_len != 1) {
                LOGE(LTAG, "Unknown OTA status len (%i)", param_len);
            }
            if (param[0] == '0') {
                LOGI(LTAG, "Update succeeded");
                if (isOtaBtClassic)
                    blueBT.pic_ota.finish(true);
                else
                    blue.pic_ota.finish(true);

            } else if (param[0] == '1') {
                LOGE(LTAG, "Update failed");
                if (isOtaBtClassic)
                    blueBT.pic_ota.finish(false);
                else
                    blue.pic_ota.finish(false);

            } else if (param[0] == '2') {
                // TODO: do something to restore the PIC
                LOGE(LTAG, "PIC stuck in bootloader");
            }
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_KN_SESS_SPK: {
            // Parse num
            param_len = pkt.nextParam(&param);
            if ((param_len == 0) || (param == NULL)) {
                LOGW(LTAG, "Param missing");
                return;
            }

            LOGI(LTAG, "Speaker has %c followers", *param);
            group.setExpectedNumMembers(*param - '0');
            if (*param == '0') {
                blue.sendGroupInfo(group);
                unlock_mut_group();
                return;
            }

            // Parse MAC
            param_len = pkt.nextParam(&param);
            if ((param_len == 0) || (param == NULL)) {
                LOGE(LTAG, "MAC param missing");
                return;
            }
            mac_ascii_t mac;
            std::copy(param, param + 12, std::begin(mac));
            // Parse KN addr
            param_len = pkt.nextParam(&param);
            if ((param_len == 0) || (param == NULL)) {
                LOGE(LTAG, "KN param missing");
                return;
            }
            spk_id_ascii_t id;
            std::copy(param, param + 6, std::begin(id));
            group.newMember(mac, id);

            if (group.isComplete()) {
                blue.sendGroupInfo(group);
                LOGI(LTAG, "space info successfully requested");
                unlock_mut_group();
            }
            break;
        }

        case NMEAPkt::NMEAPktPages::PICCOMMS_KN_SESS_SPK_EVT:
            blue.sendGroupEvent();
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_KN_SESS_SPK_FTY:
            LOGW(LTAG, "Faulty link detected");
            //TODO: send ble comm
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_VOL_MASTER:
            param_len = pkt.nextParam(&param);
            if (param_len != 2) {
                LOGE(LTAG, "Wrong param");
            } else {
                char nibbles[] = "00";
                memcpy(nibbles, param, 2);
                uint8_t vol = strtol(nibbles, NULL, 16);
                LOGI(LTAG, "Master vol = %i", vol);
                blue.sendMasterVolume(vol);
            }
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_VOL_INDIV: {
            param_len = pkt.nextParam(&param);
            uint8_t vol;
            if (param_len != 2) {
                LOGE(LTAG, "Wrong param");
                return;
            } else {
                char nibbles[] = "00";
                memcpy(nibbles, param, 2);
                vol = strtol(nibbles, NULL, 16);
            }

            param_len = pkt.nextParam(&param);
            std::array<uint8_t, 3> sid;
            if (param_len != 6) {
                LOGE(LTAG, "Wrong param");
                return;
            } else {
                char nibbles[] = "00";
                memcpy(nibbles, param, 2);
                sid[0] = strtol(nibbles, NULL, 16);
                memcpy(nibbles, param + 2, 2);
                sid[1] = strtol(nibbles, NULL, 16);
                memcpy(nibbles, param + 4, 2);
                sid[2] = strtol(nibbles, NULL, 16);
            }

            LOGI(LTAG, "Indiv vol of 0x%02X%02X%02X = %i", sid[2], sid[1],
                 sid[0], vol);
            blue.sendIndivVol(vol, sid);
            break;
        }

        case NMEAPkt::NMEAPktPages::PICCOMMS_AUDIO_CONFIG: {
            param_len = pkt.nextParam(&param);
            std::array<uint8_t, 3> settings;
            if (param_len != 6) {
                LOGE(LTAG, "Wrong param");
                return;
            } else {
                char nibbles[] = "00";
                memcpy(nibbles, param, 2);
                settings[0] = strtol(nibbles, NULL, 16);
                memcpy(nibbles, param + 2, 2);
                settings[1] = strtol(nibbles, NULL, 16);
                memcpy(nibbles, param + 4, 2);
                settings[2] = strtol(nibbles, NULL, 16);
            }

            param_len = pkt.nextParam(&param);
            std::array<uint8_t, 3> sid;
            if (param_len != 6) {
                LOGE(LTAG, "Wrong param");
                return;
            } else {
                char nibbles[] = "00";
                memcpy(nibbles, param, 2);
                sid[0] = strtol(nibbles, NULL, 16);
                memcpy(nibbles, param + 2, 2);
                sid[1] = strtol(nibbles, NULL, 16);
                memcpy(nibbles, param + 4, 2);
                sid[2] = strtol(nibbles, NULL, 16);
            }

            LOGI(LTAG, "Audio settings of 0x%02X%02X%02X = %02X, %02X, %02X",
                 sid[2], sid[1], sid[0], settings[0], settings[1], settings[2]);
            blue.sendAudioConfig(settings, sid);
            break;
        }

        case NMEAPkt::NMEAPktPages::PICCOMMS_AUDIO_PAUSE: {
            LOGI(LTAG, "Pause clocks");
            hal.i2sOut->pause();
            break;
        }

        case NMEAPkt::NMEAPktPages::PICCOMMS_AUDIO_RESUME: {
            LOGI(LTAG, "Resume clocks");
            hal.i2sOut->resume();
            break;
        }

        case NMEAPkt::NMEAPktPages::PICCOMMMS_AUDIO_EQ:
            param_len = pkt.nextParam(&param);
            if (param_len != 2) {
                LOGE(LTAG, "Wrong param");
            } else {
                char nibbles[] = "00";
                memcpy(nibbles, param, 2);
                uint8_t eq = strtol(nibbles, NULL, 16);
                LOGI(LTAG, "Equalizer = %i", eq);
                blue.sendEqualizer(eq);
            }
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC_REQ:
            // TODO: Send battery level
            LOGW(LTAG, "TODO: Send battery level");
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_TRING_MODE:
            param_len = pkt.nextParam(&param);
            if (param_len != 2) {
                LOGE(LTAG, "Wrong param");
            } else {
                char nibbles[] = "00";
                memcpy(nibbles, param, 2);
                uint8_t mode = strtol(nibbles, NULL, 16);
                LOGI(LTAG, "Touch ring mode = %i", mode);
                blue.sendTouchRingMode(mode);
            }
            break;

        case NMEAPkt::NMEAPktPages::PICCOMMS_FACTORY_RESET:
            blue.factoryReset();
            break;


        case NMEAPkt::NMEAPktPages::PICCOMMS_DWAM_STATUS:
            LOGE(LTAG, "Received DWAM status");
            param_len = pkt.nextParam(&param);
            if (param_len != 2) {
                LOGE(LTAG, "Wrong param");
            } else {
                char nibbles[] = "00";
                memcpy(nibbles, param, 2);
                uint8_t status = strtol(nibbles, NULL, 16);
                LOGI(LTAG, "DWAM status : %i", status);
                blue.sendDWAMStatus(status);
            }
            break;



        case NMEAPkt::NMEAPktPages::PICCOMMS_AUTO_POWEROFF_SUPPORT:
            LOGI(LTAG, "Enable/diisable auto power off");
            
            //Parse message 
            param_len = pkt.nextParam(&param);
            if ((param_len == 0) || (param == NULL)) {
                LOGW(LTAG, "Param missing");
                return;
            }

            if (param[0] == 'T'){
                LOGI(LTAG, "Enable auto power off");
                m_auto_pwr_off_enabled = true;   
            }
            else if(param[0] == 'F'){
                LOGI(LTAG, "Disable auto power off");
                m_auto_pwr_off_enabled = false;   
            }
            else {
                LOGW(LTAG, "Param unknown");
                m_auto_pwr_off_enabled = false;   
            }

            break;

        default:
            LOGW(LTAG, "Unknown pkt");
            break;
    }
}
