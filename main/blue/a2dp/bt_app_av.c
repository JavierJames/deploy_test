#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bt_app_av.h" 
#include "bt_app_core.h"
#include "bt_whitelist.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "driver/i2s.h"

static const char BT_AV_TAG[] = "BT_AV";
static const uint16_t UINT_16_MAX = -1;

static const float VOLUME_RAMP_SPEED = 0.02;

static float volume_mult = 0;
static bool was_suspended = true;
static bool is_conn = false;

static uint32_t m_pkt_cnt = 0;
static esp_a2d_audio_state_t m_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;

static const char* m_a2d_conn_state_str[] = {"Disconnected", "Connecting",
                                             "Connected", "Disconnecting"};
static const char* m_a2d_audio_state_str[] = {"Suspended", "Stopped",
                                              "Started"};
static bool a2d_signal_lost_mode = false;
static bool a2d_leader_mode_enabled = false;

extern bool i2s_is_paused; 

/***********************************************************************
 *                         STATIC FUNCTIONS
 ***********************************************************************/

static void _alloc_meta_buffer(esp_avrc_ct_cb_param_t* param) {
    esp_avrc_ct_cb_param_t* rc = (esp_avrc_ct_cb_param_t*)(param);
    uint8_t* attr_text = (uint8_t*)malloc(rc->meta_rsp.attr_length + 1);
    memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);
    attr_text[rc->meta_rsp.attr_length] = 0;

    rc->meta_rsp.attr_text = attr_text;
}

static void _hdl_a2d_evt(uint16_t event, void* p_param) {
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    esp_a2d_cb_param_t* a2d = (esp_a2d_cb_param_t*)p_param;

    switch (event) {
        case ESP_A2D_CONNECTION_STATE_EVT: {
            uint8_t* bda = a2d->conn_stat.remote_bda;
            ESP_LOGI(BT_AV_TAG, "A2DP conn state: %s, " ESP_BD_ADDR_STR,
                     m_a2d_conn_state_str[a2d->conn_stat.state],
                     ESP_BD_ADDR_HEX(bda));

            bt_av_update_conn_status(&a2d->conn_stat);
            bt_whitelist_flags_t* wl_flags = bt_whitelist_get_flags();
    
            if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                bt_whitelist_notify_connection(a2d->conn_stat.remote_bda);
                wl_flags->should_reconnect = true;
                is_conn = true;
                a2d_signal_lost_mode = false;
            } 
            else if(a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING){ 
                /* check for new connection of other device during a signal loss */
                if(a2d_signal_lost_mode)
                {
                    esp_bd_addr_t recent_bda;
                    bt_whitelist_last(recent_bda);
                    
                    if(!bt_whitelist_compare_bda(a2d->conn_stat.remote_bda, recent_bda))
                    {
                        ESP_LOGI(BT_AV_TAG, "New connection, aborting signal lost mode");
                        a2d_signal_lost_mode = false;
                    }
                }

                break;
            }
            else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
                /* abnormal disconnect happens when disconnect is casued by signal loss, try reconnect */
                is_conn = false;
                if(a2d->conn_stat.disc_rsn == ESP_A2D_DISC_RSN_ABNORMAL || a2d_signal_lost_mode)
                {
                    ESP_LOGW(BT_AV_TAG, "Disconnected due to signal loss");

                    /* TODO: polling for reconnection on signal lost
                        should be replaced with an event based system when 
                        possible */

                    //a2d_signal_lost_mode = bt_app_a2d_reconnect_last_dev();

                    if(a2d_signal_lost_mode) 
                        break;
                }
                else if(!a2d_leader_mode_enabled)
                    break; 
                else
                    wl_flags->should_reconnect = false;

                ESP_LOGI(BT_AV_TAG, "Re-start GAP scan as connectable");
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            } 

            bt_whitelist_save_flags();

            break;
        }
        case ESP_A2D_AUDIO_STATE_EVT: {
            ESP_LOGI(BT_AV_TAG, "A2DP audio state: %s",
                     m_a2d_audio_state_str[a2d->audio_stat.state]);
            m_audio_state = a2d->audio_stat.state;
            if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
                m_pkt_cnt = 0;
            }
            if(ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND == a2d->audio_stat.state || ESP_A2D_AUDIO_STATE_STOPPED == a2d->audio_stat.state)
            {    
                was_suspended = true;
            }
            break;
        }
        case ESP_A2D_AUDIO_CFG_EVT: {
            ESP_LOGI(BT_AV_TAG,
                     "A2DP audio stream configuration, codec type %d",
                     a2d->audio_cfg.mcc.type);
            // for now only SBC stream is supported
            if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
                int sample_rate = 16000;
                char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
                if (oct0 & (0x01 << 6)) {
                    sample_rate = 32000;
                } else if (oct0 & (0x01 << 5)) {
                    sample_rate = 44100;
                } else if (oct0 & (0x01 << 4)) {
                    sample_rate = 48000;
                }
                i2s_set_clk(0, sample_rate, 32, 2);

                ESP_LOGI(BT_AV_TAG, "configure audio player %x-%x-%x-%x",
                         a2d->audio_cfg.mcc.cie.sbc[0],
                         a2d->audio_cfg.mcc.cie.sbc[1],
                         a2d->audio_cfg.mcc.cie.sbc[2],
                         a2d->audio_cfg.mcc.cie.sbc[3]);
                ESP_LOGI(BT_AV_TAG, "audio player configured, samplerate=%d",
                         sample_rate);
            }
            break;
        }
        default:
            ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
            break;
    }
}

static void _new_track() {
    // Register notifications and request metadata
    esp_avrc_ct_send_metadata_cmd(
        0, ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST |
               ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_GENRE);
    esp_avrc_ct_send_register_notification_cmd(1, ESP_AVRC_RN_TRACK_CHANGE, 0);
}

static void _notify_evt_handler(uint8_t event_id, esp_avrc_rn_param_t event_parameter) {
    switch (event_id) {
        case ESP_AVRC_RN_TRACK_CHANGE:
            _new_track();
            break;
    }
}

static void _hdl_avrc_evt(uint16_t event, void* p_param) {
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    esp_avrc_ct_cb_param_t* rc = (esp_avrc_ct_cb_param_t*)(p_param);
    switch (event) {
        case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
            uint8_t* bda = rc->conn_stat.remote_bda;
            ESP_LOGI(BT_AV_TAG, "AVRCP conn state: %d, " ESP_BD_ADDR_STR,
                     rc->conn_stat.connected, ESP_BD_ADDR_HEX(bda));

            if (rc->conn_stat.connected) {

                _new_track();
            }
            break;
        }
        case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT: {
            ESP_LOGI(BT_AV_TAG,
                     "AVRCP passthrough rsp: key_code 0x%x, key_state %d",
                     rc->psth_rsp.key_code, rc->psth_rsp.key_state);
            break;
        }
        case ESP_AVRC_CT_METADATA_RSP_EVT: {
            ESP_LOGI(BT_AV_TAG, "AVRCP metadata rsp: attribute id 0x%x, %s",
                     rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
            free(rc->meta_rsp.attr_text);
            break;
        }
        case ESP_AVRC_CT_CHANGE_NOTIFY_EVT: {
            ESP_LOGI(BT_AV_TAG, "AVRCP event notification: %d", rc->change_ntf.event_id);
            _notify_evt_handler(rc->change_ntf.event_id, rc->change_ntf.event_parameter);
            break;
        }
        case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
            ESP_LOGI(BT_AV_TAG, "AVRCP remote features %x",
                     rc->rmt_feats.feat_mask);
            break;
        }
        default:
            ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
            break;
    }
}

//**********************************************************************
//                         GLOBAL FUNCTIONS
//**********************************************************************

void bt_a2d_set_signal_lost_mode(bool mode) {
   a2d_signal_lost_mode = mode; 
}

void bt_a2d_set_leader_mode_enabled(bool mode) {
    a2d_leader_mode_enabled = mode;
}

bool bt_a2d_get_leader_mode_enabled() {
    return a2d_leader_mode_enabled;
}

// fills the dma buffer with zero's to hide glitches that occur in the beginning and to prevent the buffer from getting empty (this would also cause glitches)
void zero_fill_dma() {
    size_t bytes_written;
    uint32_t zero_buffer_len = 1028;
    char zero_buffer[zero_buffer_len];

    memset(zero_buffer, 0, zero_buffer_len);

    if(i2s_is_paused)
        return;

    // dma buffer is dynamic, so we have to call write expand multiple times
    for(int i = 0; i < 20; i++)
    {
        i2s_write_expand(0, zero_buffer, zero_buffer_len, 16, 32, &bytes_written, portMAX_DELAY);
    } 
}

// ramps up the volume after the A2DP connection has been stopped or suspended
void ramp_volume(uint8_t* data, uint32_t len) {
    if(was_suspended)
        volume_mult = 0;

    if(volume_mult < 1)
        volume_mult += VOLUME_RAMP_SPEED;

    // convert uint8_t to int16_t to apply volume multiplication and convert back to uint8_t
    for(int i = 0; i < len; i+=2)
    {
        uint16_t combined_val = (data[i+1] << 8 ) | (data[i] & 0xff);
        int16_t signed_val = combined_val + UINT_16_MAX + 1;
        signed_val *= volume_mult;
        uint16_t unsigned_val = signed_val - UINT_16_MAX - 1;
        data[i] = unsigned_val & 0x00FF;
        data[i+1] = unsigned_val >> 8;
    }
}

void bt_a2d_close() {
    if(!is_conn)
        return;

    ESP_LOGI(BT_AV_TAG, "Disconnecting a2d sink");

    esp_bd_addr_t bda;
    bt_whitelist_last(bda);
    esp_err_t err = esp_a2d_sink_disconnect(bda);
    if(err)
        ESP_LOGE(BT_AV_TAG, "Cannot disconnect sink (%s)", esp_err_to_name(err));

    err = esp_a2d_sink_deinit();
    if(err)
        ESP_LOGE(BT_AV_TAG, "A2D sink deinit failed (%s)", esp_err_to_name(err));

    err = esp_avrc_ct_deinit();
    if(err)
        ESP_LOGE(BT_AV_TAG, "A2D AVRC deinit failed (%s)", esp_err_to_name(err));

    sleep(3);
}

void bt_app_a2d_data_cb(uint8_t* data, uint32_t len) {
    size_t bytes_written;

    ramp_volume(data, len);
    if(was_suspended)
    {
        zero_fill_dma();
        was_suspended = false;
    }

    if(i2s_is_paused)
        return;

    i2s_write_expand(0, data, len, 16, 32, &bytes_written, portMAX_DELAY);
    if (++m_pkt_cnt % 100 == 0)
        ESP_LOGD(BT_AV_TAG, "audio data pkt cnt %u", m_pkt_cnt);
}

void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* param) {
    switch (event) {
        case ESP_A2D_CONNECTION_STATE_EVT:
        case ESP_A2D_AUDIO_STATE_EVT:
        case ESP_A2D_AUDIO_CFG_EVT: {
            bt_app_work_dispatch(_hdl_a2d_evt, event, param,
                                 sizeof(esp_a2d_cb_param_t), NULL);
            break;
        }
        case ESP_A2D_MEDIA_CTRL_ACK_EVT:
            ESP_LOGI(BT_AV_TAG, "a2dp ctrl event acknowledged");
            break;
        default:
            ESP_LOGE(BT_AV_TAG, "a2dp invalid cb event: %d", event);
            break;
    }
}

void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t* param) {
    switch (event) {
        case ESP_AVRC_CT_METADATA_RSP_EVT:
            _alloc_meta_buffer(param);
        case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
        case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
            bt_app_work_dispatch(_hdl_avrc_evt, event, param,
                                 sizeof(esp_avrc_ct_cb_param_t), NULL);
            break;
        }
        default:
            ESP_LOGE(BT_AV_TAG, "avrc invalid cb event: %d", event);
            break;
    }
}
