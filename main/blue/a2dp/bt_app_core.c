#include "bt_app_core.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/xtensa_api.h"

#include "bt_app_av.h"
#include "bt_whitelist.h"

#define BT_APP_SIG_WORK_DISPATCH (0x01)

static const char BT_APP_CORE_TAG[] = "BT_AV_CORE";

static xQueueHandle bt_app_task_queue = NULL;
static xTaskHandle bt_app_task_handle = NULL;
static xSemaphoreHandle bt_app_link_evt = NULL;

static struct a2d_conn_stat_param m_current_conn;


/***********************************************************************
 *                         STATIC FUNCTIONS
 ***********************************************************************/

static void _gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) {
    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(BT_APP_CORE_TAG, "Auth success: %s",
                         param->auth_cmpl.device_name);
                esp_log_buffer_hex(BT_APP_CORE_TAG, param->auth_cmpl.bda,
                                   ESP_BD_ADDR_LEN);
            } else {
                ESP_LOGE(BT_APP_CORE_TAG, "Auth failed, status:%d",
                         param->auth_cmpl.stat);
            }
            break;
        case ESP_BT_GAP_CFM_REQ_EVT:
            ESP_LOGI(BT_APP_CORE_TAG, "Compare the numeric value: %d",
                     param->cfm_req.num_val);
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;
        case ESP_BT_GAP_KEY_NOTIF_EVT:
            ESP_LOGI(BT_APP_CORE_TAG, "Passkey: %d", param->key_notif.passkey);
            break;
        case ESP_BT_GAP_KEY_REQ_EVT:
            ESP_LOGI(BT_APP_CORE_TAG, "Enter passkey!");
            break;
        case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
            break;
        default:
            ESP_LOGW(BT_APP_CORE_TAG, "Unhandled GAP event: %d", event);
            break;
    }
    return;
}

static bool _send_msg(bt_app_msg_t* msg) {
    if ((msg == NULL) || (bt_app_task_queue == NULL)) {
        ESP_LOGE(BT_APP_CORE_TAG, "%s app send failed", __func__);
        return false;
    }

    if (xQueueSend(bt_app_task_queue, msg, 10 / portTICK_RATE_MS) != pdTRUE) {
        ESP_LOGE(BT_APP_CORE_TAG, "%s xQueue send failed", __func__);
        return false;
    }
    return true;
}

static void _work_dispatched(bt_app_msg_t* msg) {
    if (msg->cb) {
        msg->cb(msg->event, msg->param);
    }
}

static void _task_handler(void* arg) {
    bt_app_msg_t msg;
    for (;;) {
        if (pdTRUE == xQueueReceive(bt_app_task_queue, &msg,
                                    (portTickType)portMAX_DELAY)) {
            ESP_LOGD(BT_APP_CORE_TAG, "%s, sig 0x%x, 0x%x", __func__, msg.sig,
                     msg.event);
            switch (msg.sig) {
                case BT_APP_SIG_WORK_DISPATCH:
                    _work_dispatched(&msg);
                    break;
                default:
                    ESP_LOGW(BT_APP_CORE_TAG, "%s, unhandled sig: %d", __func__,
                             msg.sig);
                    break;
            }  // switch (msg.sig)

            if (msg.param) {
                free(msg.param);
            }
        }
    }
}

/***********************************************************************
 *                         GLOBAL FUNCTIONS
 ***********************************************************************/

bool bt_app_a2d_reconnect_last_dev() {

    esp_bd_addr_t recent_bda;
    bt_whitelist_flags_t* wl_flags = bt_whitelist_get_flags();

    if(!wl_flags->should_reconnect)
        return false;

    if (bt_whitelist_size() < 1) 
        return false;

    if (!bt_a2d_get_leader_mode_enabled())
        return false;

    ESP_LOGI(BT_APP_CORE_TAG, "Reconnecting to last device");

    int real_devs_num = esp_bt_gap_get_bond_device_num();
    esp_bd_addr_t dev;
    ESP_LOGI(BT_APP_CORE_TAG, "Amount of paired devices: %d", real_devs_num);
    
    bt_whitelist_last(recent_bda);

    esp_err_t err = esp_a2d_sink_init();
    if(err != ESP_OK)
        return false;

    ESP_LOGI(BT_APP_CORE_TAG,"A2DP connecting to: %02x:%02x:%02x:%02x:%02x:%02x", ESP_BD_ADDR_HEX(recent_bda));
    if (esp_a2d_sink_connect(recent_bda) == ESP_FAIL) {
        ESP_LOGI(BT_APP_CORE_TAG, "Couldn't send A2DP connect to lower layer");
    } else ESP_LOGI(BT_APP_CORE_TAG, "A2DP connect command sent to lower layer");

    return true;
}

void bt_av_update_conn_status(struct a2d_conn_stat_param* conn) {
    assert(conn);

    memcpy(&m_current_conn, conn, sizeof(m_current_conn));
    xSemaphoreGive(bt_app_link_evt);
}

esp_a2d_connection_state_t bt_av_get_conn_status() {
    return m_current_conn.state;
}

/* handler for bluetooth stack enabled events */
void bt_av_hdl_stack_evt(uint16_t event, void* p_param) {
    ESP_LOGD(BT_APP_CORE_TAG, "%s evt %d", __func__, event);
    switch (event) {
        case BT_APP_EVT_STACK_UP: {
            ESP_LOGI(BT_APP_CORE_TAG, "BT on");

            /* configure pairing policy */
            esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
            esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
            esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

            esp_bt_gap_register_callback(&_gap_cb);

            /* initialize A2DP sink */
            esp_a2d_register_callback(&bt_app_a2d_cb);
            esp_a2d_sink_register_data_callback(&bt_app_a2d_data_cb);
            esp_a2d_sink_init();

            /* initialize AVRCP controller */
            esp_avrc_ct_init();
            esp_avrc_ct_register_callback(bt_app_rc_ct_cb);

            /* set neither connectable mode nor discoverable */
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            break;
        }

        case BT_APP_EVT_CONNECTABLE_DISCOVERABLE: {
            if (m_current_conn.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                esp_a2d_sink_disconnect(m_current_conn.remote_bda);
            }

            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            ESP_LOGI(BT_APP_CORE_TAG, "Device discoverable");
            break;
        }
        // Taken out on Fri 17th Apr - Consider modifying this to put and call all written functions here
        case BT_APP_EVT_CONNECTABLE: {
            if (!bt_app_a2d_reconnect_last_dev()) {
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
                ESP_LOGI(BT_APP_CORE_TAG, "Device connectable");
            }
            break;
        }
        
        case BT_APP_EVT_STACK_DOWN: {
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            esp_err_t err = esp_a2d_sink_deinit();
            if(err)
                ESP_LOGE(BT_APP_CORE_TAG, "A2D sink deinit failed (%s)", esp_err_to_name(err));

            err = esp_avrc_ct_deinit();
            if(err)
                ESP_LOGE(BT_APP_CORE_TAG, "A2D AVRC deinit failed (%s)", esp_err_to_name(err));
            
            ESP_LOGI(BT_APP_CORE_TAG, "Device not connectable");
            break;
        }

        case BT_APP_EVT_DISCONNECT: {
            if (m_current_conn.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                ESP_LOGI(BT_APP_CORE_TAG,"Disconnecting sink for: %02x:%02x:%02x:%02x:%02x:%02x", ESP_BD_ADDR_HEX(m_current_conn.remote_bda));

                esp_err_t err = esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_STOP);
                if(err)
                    ESP_LOGE(BT_APP_CORE_TAG, "A2D media ctrl stop failed (%s)", esp_err_to_name(err));

                err = esp_a2d_sink_disconnect(m_current_conn.remote_bda);
                if(err)
                    ESP_LOGE(BT_APP_CORE_TAG, "A2D sink disconnect failed (%s)", esp_err_to_name(err));
            }
            break;
        }
        
        default:
            ESP_LOGE(BT_APP_CORE_TAG, "%s unhandled evt %d", __func__, event);
            break;
    }
}

bool bt_app_work_dispatch(bt_app_cb_t p_cback,
                          uint16_t event,
                          void* p_params,
                          int param_len,
                          bt_app_copy_cb_t p_copy_cback) {
    ESP_LOGD(BT_APP_CORE_TAG, "%s event 0x%x, param len %d", __func__, event,
             param_len);

    bt_app_msg_t msg;
    memset(&msg, 0, sizeof(bt_app_msg_t));

    msg.sig = BT_APP_SIG_WORK_DISPATCH;
    msg.event = event;
    msg.cb = p_cback;

    if (param_len == 0) {
        return _send_msg(&msg);
    } else if (p_params && param_len > 0) {
        if ((msg.param = malloc(param_len)) != NULL) {
            memcpy(msg.param, p_params, param_len);
            /* check if caller has provided a copy callback to do the deep copy
             */
            if (p_copy_cback) {
                p_copy_cback(&msg, msg.param, p_params);
            }
            return _send_msg(&msg);
        }
    }

    return false;
}

void bt_app_task_start_up(xSemaphoreHandle link_evt) {
    bt_app_link_evt = link_evt;
    m_current_conn.state = ESP_A2D_CONNECTION_STATE_DISCONNECTED;
    bt_app_task_queue = xQueueCreate(10, sizeof(bt_app_msg_t));
    xTaskCreate(_task_handler, "BtAppT", 2500, NULL, configMAX_PRIORITIES - 3,
                &bt_app_task_handle);
}

void bt_app_task_shut_down(void) {
    if (bt_app_task_handle) {
        vTaskDelete(bt_app_task_handle);
        bt_app_task_handle = NULL;
    }
    if (bt_app_task_queue) {
        vQueueDelete(bt_app_task_queue);
        bt_app_task_queue = NULL;
    }
    m_current_conn.state = ESP_A2D_CONNECTION_STATE_DISCONNECTED;
}
