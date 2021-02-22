#include "ble_prof_nus.h"

#include <stdint.h>
#include <string.h>
#include "esp_log.h"

#include "esp_gap_ble_api.h"

static const char LTAG[] = "BLE_NUS";

#define NUS_DATA_RECV_MAX_LEN 20
#define NUS_DATA_SEND_MAX_LEN 20

static xQueueHandle _rx_queue = NULL;
static nus_pkt_t _rxbuff;

static uint16_t _mtu_size = 23;
static uint16_t _conn_id = 0xffff;
static esp_gatt_if_t _gatts_if = 0xff;

static bool _send_notify = false;
static bool _is_connected = false;
static esp_bd_addr_t _remote_bda = {0x0};

/*****************************************************************************
 *  NUS PROFILE ATTRIBUTES >>>
 *****************************************************************************/

// NUS Attributes Table
enum {
    NUS_IDX_SVC,

    NUS_IDX_DATA_RECV_CHAR,
    NUS_IDX_DATA_RECV_VAL,

    NUS_IDX_DATA_SEND_CHAR,
    NUS_IDX_DATA_SEND_VAL,
    NUS_IDX_DATA_SEND_CFG,

    NUS_IDX_NB  // Total number of attributes. This shall be the last item.
};

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid =
    ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const uint8_t nus_service_uuid[ESP_UUID_LEN_128] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E};

/// NUS Service - data receive characteristic, write without response
static const uint8_t nus_char_rx_uuid[ESP_UUID_LEN_128] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E};
static const uint8_t nus_data_receive_val[20] = {0x00};

/// NUS Service - status characteristic, notify & read
static const uint8_t nus_send_uuid[ESP_UUID_LEN_128] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E};
static const uint8_t nus_send_val[10] = {0x00};
static const uint8_t nus_send_ccc[2] = {0x00, 0x00};

/// Full NUS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t _kien_gatt_db[NUS_IDX_NB] = {
    // NUS -  Service Declaration
    [NUS_IDX_SVC] = {{ESP_GATT_AUTO_RSP},
                     {ESP_UUID_LEN_16, (uint8_t*)&primary_service_uuid,
                      ESP_GATT_PERM_READ, sizeof(nus_service_uuid),
                      sizeof(nus_service_uuid), (uint8_t*)nus_service_uuid}},

    // NUS -  RX characteristic Declaration
    [NUS_IDX_DATA_RECV_CHAR] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16,
                                 (uint8_t*)&character_declaration_uuid,
                                 ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE,
                                 CHAR_DECLARATION_SIZE,
                                 (uint8_t*)&char_prop_write}},
    // NUS -  RX characteristic Value
    [NUS_IDX_DATA_RECV_VAL] = {{ESP_GATT_AUTO_RSP},
                               {ESP_UUID_LEN_128, (uint8_t*)nus_char_rx_uuid,
                                ESP_GATT_PERM_WRITE, NUS_DATA_RECV_MAX_LEN,
                                sizeof(nus_data_receive_val),
                                (uint8_t*)nus_data_receive_val}},

    // NUS -  RX characteristic Declaration
    [NUS_IDX_DATA_SEND_CHAR] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16,
                                 (uint8_t*)&character_declaration_uuid,
                                 ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE,
                                 CHAR_DECLARATION_SIZE,
                                 (uint8_t*)&char_prop_notify}},
    // NUS -  RX characteristic Value
    [NUS_IDX_DATA_SEND_VAL] = {{ESP_GATT_AUTO_RSP},
                               {ESP_UUID_LEN_128, (uint8_t*)&nus_send_uuid,
                                ESP_GATT_PERM_READ, NUS_DATA_SEND_MAX_LEN,
                                sizeof(nus_send_val), (uint8_t*)nus_send_val}},
    // NUS -  RX characteristic - Client Characteristic Configuration Descriptor
    [NUS_IDX_DATA_SEND_CFG] = {{ESP_GATT_AUTO_RSP},
                               {ESP_UUID_LEN_16,
                                (uint8_t*)&character_client_config_uuid,
                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                sizeof(uint16_t), sizeof(nus_send_ccc),
                                (uint8_t*)nus_send_ccc}},
};

/*****************************************************************************
 *  <<< NUS PROFILE ATTRIBUTES
 *****************************************************************************/

static uint16_t _kien_handle_table[NUS_IDX_NB];

static uint8_t _find_char_and_desr_index(uint16_t handle) {
    uint8_t error = 0xff;

    for (int i = 0; i < NUS_IDX_NB; i++) {
        if (handle == _kien_handle_table[i]) {
            return i;
        }
    }

    return error;
}

void nus_evt_handlr(esp_gatts_cb_event_t event,
                    esp_gatt_if_t gatts_if,
                    esp_ble_gatts_cb_param_t* param) {
    esp_ble_gatts_cb_param_t* p_data = (esp_ble_gatts_cb_param_t*)param;
    if (param == NULL) {
        ESP_LOGE(LTAG, "Callback *param is NULL");
        return;
    }

    if (event == ESP_GATTS_REG_EVT) {
        esp_err_t create_attr_ret =
            esp_ble_gatts_create_attr_tab(_kien_gatt_db, gatts_if, NUS_IDX_NB,
                                          NUS_IDX_SVC);  // Only one service
        if (create_attr_ret != ESP_OK) {
            ESP_LOGE(LTAG, "Attr table not created, err 0x%x", create_attr_ret);
        } else {
            ESP_LOGI(LTAG, "Attr table created");
        }
    } else if (event == ESP_GATTS_READ_EVT) {
        ESP_LOGW(LTAG, "ESP_GATTS_READ_EVT");
    } else if (event == ESP_GATTS_WRITE_EVT) {
        uint8_t res = _find_char_and_desr_index(p_data->write.handle);
        if (p_data->write.is_prep == false) {
            ESP_LOGD(LTAG, "ESP_GATTS_WRITE_EVT : handle = %d", res);
            if (res == NUS_IDX_DATA_RECV_VAL) {
                memcpy(_rxbuff.data, p_data->write.value, p_data->write.len);
                _rxbuff.len = p_data->write.len;
                if (_rx_queue == NULL || _rx_queue == 0) {
                    ESP_LOGE(LTAG, "Queue has not been initialized");
                } else {
                    xQueueSend(_rx_queue, &_rxbuff, 10 / portTICK_PERIOD_MS);
                }
            } else if (res == NUS_IDX_DATA_SEND_CFG) {
                if ((p_data->write.len == 2) &&
                    (p_data->write.value[0] == 0x01) &&
                    (p_data->write.value[1] == 0x00)) {
                    _send_notify = true;
                } else if ((p_data->write.len == 2) &&
                           (p_data->write.value[0] == 0x00) &&
                           (p_data->write.value[1] == 0x00)) {
                    _send_notify = false;
                }
            } else {
                ESP_LOGE(LTAG, "Unknown write attr");
            }
        } else if (p_data->write.is_prep == true) {
            ESP_LOGE(LTAG, "ESP_GATTS_PREP_WRITE_EVT : handle = %d", res);
        }
    } else if (event == ESP_GATTS_EXEC_WRITE_EVT) {
        ESP_LOGI(LTAG, "ESP_GATTS_EXEC_WRITE_EVT");
    } else if (event == ESP_GATTS_MTU_EVT) {
        if (p_data->mtu.mtu > NUS_MTU_MAX) {
            ESP_LOGW(LTAG, "MTU bigger than available (%i)", p_data->mtu.mtu);
        } else {
            _mtu_size = p_data->mtu.mtu;
            ESP_LOGI(LTAG, "New MTU %i", _mtu_size);
        }
    } else if (event == ESP_GATTS_CONNECT_EVT) {
        ESP_LOGI(LTAG, "ESP_GATTS_CONNECT_EVT");
        _conn_id = p_data->connect.conn_id;
        _gatts_if = gatts_if;
        _is_connected = true;
        memcpy(&_remote_bda, &p_data->connect.remote_bda,
               sizeof(esp_bd_addr_t));

        // Request to update the connection parameters to increase throughput
        esp_ble_conn_update_params_t conn_params;
        memcpy(conn_params.bda, _remote_bda, ESP_BD_ADDR_LEN);
        conn_params.min_int = 0x06;  // x 1.25ms
        conn_params.max_int = 0x20;  // x 1.25ms
        conn_params.latency = 0x00;  // number of skippable connection events
        conn_params.timeout = 0xA0;  // x 6.25ms, time before peripheral will
                                     // assume connection is dropped.
        esp_ble_gap_update_conn_params(&conn_params);

    } else if (event == ESP_GATTS_DISCONNECT_EVT) {
        ESP_LOGI(LTAG, "ESP_GATTS_DISCONNECT_EVT");
        _send_notify = false;
        _is_connected = false;

        // Abort ongoing OTA updates
        _rxbuff.data[0] = BLE_NUS_CMD_OTA_ABORT;
        _rxbuff.len = 1;
        if (_rx_queue == NULL || _rx_queue == 0) {
            ESP_LOGE(LTAG, "Queue has not been initialized");
        } else {
            xQueueSend(_rx_queue, &_rxbuff, 10 / portTICK_PERIOD_MS);
        }
    } else if (event == ESP_GATTS_CREAT_ATTR_TAB_EVT) {
        ESP_LOGD(LTAG, "Num handle = %x", param->add_attr_tab.num_handle);
        if (param->add_attr_tab.status != ESP_GATT_OK) {
            ESP_LOGE(LTAG, "Create att table failed, err 0x%x",
                     param->add_attr_tab.status);
        } else if (param->add_attr_tab.num_handle != NUS_IDX_NB) {
            ESP_LOGE(LTAG,
                     "Create att table abnormally, num_handle (%d) "
                     "differs from NUS_IDX_NB (%d)",
                     param->add_attr_tab.num_handle, NUS_IDX_NB);
        } else {
            memcpy(_kien_handle_table, param->add_attr_tab.handles,
                   sizeof(_kien_handle_table));
            esp_ble_gatts_start_service(_kien_handle_table[NUS_IDX_SVC]);
            ESP_LOGI(LTAG, "NUS ready");
        }
    } else if (event == ESP_GATTS_CONF_EVT) {
        ESP_LOGI(LTAG, "ESP_GATTS_CONF_EVT, status %d", param->conf.status);
    } else {
        ESP_LOGW(LTAG, "GATT_EVT 0x%x not handled", event);
    }
}

void ble_nus_send(uint8_t* buff, size_t len) {
    if (len == 0) {
        ESP_LOGW(LTAG, "NUS data len empty");
    } else {
        esp_ble_gatts_send_indicate(_gatts_if, _conn_id,
                                    _kien_handle_table[NUS_IDX_DATA_SEND_VAL],
                                    len, (uint8_t*)buff, false);
    }
}

void ble_nus_init(xQueueHandle rx_cmd_queue) {
    if (rx_cmd_queue == NULL || rx_cmd_queue == 0) {
        ESP_LOGE(LTAG, "Queue has to be created by client");
    } else {
        _rx_queue = rx_cmd_queue;
    }
}
