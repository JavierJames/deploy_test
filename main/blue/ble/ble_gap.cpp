#include "ble_gap.hpp"

#include "esp_log.h"

static const char LTAG[] = "BLE_GAP";

esp_gap_ble_cb_t peripheral_evt_callback = nullptr;
esp_gap_ble_cb_t central_evt_callback = nullptr;

static void _evt_handler(esp_gap_ble_cb_event_t event,
                         esp_ble_gap_cb_param_t* param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            if (peripheral_evt_callback != nullptr)
                peripheral_evt_callback(event, param);
            else
                ESP_LOGW(LTAG, "Peripheral not registered");
            break;

        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        case ESP_GAP_BLE_SCAN_RESULT_EVT:
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if (central_evt_callback != nullptr)
                central_evt_callback(event, param);
            else
                ESP_LOGW(LTAG, "Central not registered");
            break;

        default:
            ESP_LOGW(LTAG, "GAP_EVT 0x%x not handled", event);
            break;
    }
}

Result Kien::ble_gap_init() {
    esp_err_t res = esp_ble_gap_register_callback(_evt_handler);
    if (res != ESP_OK) {
        ESP_LOGE(LTAG, "GAP cb not set, err %x", res);
        return Result::Fail;
    } else {
        return Result::Ok;
    }
}

void Kien::ble_gap_register_peripheral(esp_gap_ble_cb_t callback) {
    peripheral_evt_callback = callback;
}

void Kien::ble_gap_register_central(esp_gap_ble_cb_t callback) {
    central_evt_callback = callback;
}
