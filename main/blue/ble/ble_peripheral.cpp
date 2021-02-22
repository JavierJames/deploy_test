#include "ble_peripheral.hpp"

#include "esp_log.h"

static const char LTAG[] = "BLE_PERIPH";

 
/* Notes on values that work for BLE ADV TIME MIN and MAX
*  are listed belowe as min-max. 
* 200-600: fails
* 200-300: fails after 30 minutes
* 100-200: fails after 27 minutes
* 50-70: works over 32 minutes
* 50-10: works over 47 minutes
*/
#define BLE_ADV_TIME_MIN_MS 40  // 20 ms to 10.24 sec
#define BLE_ADV_TIME_MAX_MS 60  // 20 ms to 10.24 sec  
#define BLE_TIME_MS_TO_VALUE(x) (int)(x / 0.625)

static uint8_t adv_config_status = 0;
#define ADV_CONFIG_ADV_FLAG (1 << 0)
#define ADV_CONFIG_SCAN_RSP_FLAG (1 << 1)

void Kien::ble_peripheral_evt_handler(esp_gap_ble_cb_event_t event,
                                      esp_ble_gap_cb_param_t* param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_status &= (~ADV_CONFIG_ADV_FLAG);
            if (adv_config_status == 0) {
                Kien::ble_peripheral_start_adv();
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_status &= (~ADV_CONFIG_SCAN_RSP_FLAG);
            if (adv_config_status == 0) {
                Kien::ble_peripheral_start_adv();
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            assert(param);
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(LTAG, "Advertising start failed");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            Kien::ble_peripheral_start_adv();
            break;
        default:
            ESP_LOGW(LTAG, "GAP_EVT 0x%x not handled", event);
            break;
    }
}

Result Kien::ble_peripheral_start_adv() {
    esp_ble_adv_params_t adv_params;
    adv_params.adv_int_min = BLE_TIME_MS_TO_VALUE(BLE_ADV_TIME_MIN_MS);
    adv_params.adv_int_max = BLE_TIME_MS_TO_VALUE(BLE_ADV_TIME_MAX_MS);
    adv_params.adv_type = ADV_TYPE_IND;
    adv_params.peer_addr_type = BLE_ADDR_TYPE_PUBLIC;
    adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    adv_params.channel_map = ADV_CHNL_ALL;
    adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

    esp_err_t res = esp_ble_gap_start_advertising(&adv_params);
    if (res != ESP_OK) {
        ESP_LOGE(LTAG, "Failed to start adv, err %x", res);
        return Result::Fail;
    } else {
        return Result::Ok;
    }
}

Result Kien::ble_peripheral_stop_adv() {
    esp_err_t res = esp_ble_gap_stop_advertising();
    if (res != ESP_OK) {
        ESP_LOGE(LTAG, "Failed to stop adv, err %x", res);
        return Result::Fail;
    } else {
        return Result::Ok;
    }
}

Result Kien::ble_peripheral_set_advertisement(uint8_t* data, uint8_t len) {
    esp_err_t res = esp_ble_gap_config_adv_data_raw(data, len);
    if (res != ESP_OK) {
        ESP_LOGE(LTAG, "Failed to set adv data, err %x", res);
        return Result::Fail;
    } else {
        adv_config_status |= ADV_CONFIG_ADV_FLAG;
        return Result::Ok;
    }
}

Result Kien::ble_peripheral_set_scan_response(uint8_t* data, uint8_t len) {
    esp_err_t res = esp_ble_gap_config_scan_rsp_data_raw(data, len);
    if (res != ESP_OK) {
        ESP_LOGE(LTAG, "Failed to set scan_res data, err %x", res);
        return Result::Fail;
    } else {
        adv_config_status |= ADV_CONFIG_SCAN_RSP_FLAG;
        return Result::Ok;
    }
}
