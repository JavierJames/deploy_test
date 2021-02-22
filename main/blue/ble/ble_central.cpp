#include "ble_central.hpp"
#include "uta/dataset.hpp"

#include <cassert>
#include <cstring>

#include "esp_bt_device.h"
#include "esp_log.h"

#include "ble_adv_conf.hpp"

#define BLE_SCAN_INTERVAL_MS 100
#define BLE_SCAN_WINDOW_MS 70
#define BLE_TIME_MS_TO_VALUE(x) (int)(x / 0.625)

static const char LTAG[] = "BLE_CENTRAL";

using namespace Kien;

void Kien::ble_central_evt_handler(esp_gap_ble_cb_event_t event,
                                   esp_ble_gap_cb_param_t* param) {
    ds_uta_t ds;

    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            Kien::ble_central_start_scan(0);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
            assert(param);
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(LTAG, "Scanning start failed");
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            assert(param);
            esp_ble_gap_cb_param_t* scan_result =
                (esp_ble_gap_cb_param_t*)param;

            if (scan_result->scan_rst.search_evt ==
                ESP_GAP_SEARCH_INQ_RES_EVT) {
                if (Kien::ble_adv_is_altbeacon(
                        scan_result->scan_rst.ble_adv,
                        scan_result->scan_rst.adv_data_len)) {
                    ESP_LOGI(LTAG, "Kien beacon found");

                    // We can rely on casting now because we are sure it's a
                    // Kien beacon
                    Kien::altbeacon_t* beacon =
                        reinterpret_cast<Kien::altbeacon_t*>(
                            scan_result->scan_rst.ble_adv);
                    Kien::ds_parse_beacon(ds, *beacon,
                                          scan_result->scan_rst.rssi);

                    Kien::scan_response_t* scanres =
                        reinterpret_cast<Kien::scan_response_t*>(
                            scan_result->scan_rst.ble_adv);
                    Kien::ds_parse_scanres(ds, *scanres);

                    Kien::ds_get_rx_queue().push(ds);
                }
            }
            break;
        }
        default:
            ESP_LOGW(LTAG, "GAP_EVT 0x%x not handled", event);
            break;
    }
}

Result Kien::ble_central_start_scan(int duration) {
    esp_err_t res = esp_ble_gap_start_scanning(duration);
    if (res != ESP_OK) {
        ESP_LOGE(LTAG, "Failed to start adv, err %x", res);
        return Result::Fail;
    } else {
        return Result::Ok;
    }
}

Result Kien::ble_central_stop_scan() {
    esp_err_t res = esp_ble_gap_stop_scanning();
    if (res != ESP_OK) {
        ESP_LOGE(LTAG, "Failed to stop adv, err %x", res);
        return Result::Fail;
    } else {
        return Result::Ok;
    }
}

Result Kien::ble_central_set_scan_params() {
    static esp_ble_scan_params_t ble_scan_params;
    ble_scan_params.scan_type = BLE_SCAN_TYPE_ACTIVE;
    ble_scan_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    ble_scan_params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
    ble_scan_params.scan_interval = BLE_TIME_MS_TO_VALUE(BLE_SCAN_WINDOW_MS);
    ble_scan_params.scan_window = BLE_TIME_MS_TO_VALUE(BLE_SCAN_WINDOW_MS);
    ble_scan_params.scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE;

    esp_err_t res = esp_ble_gap_set_scan_params(&ble_scan_params);
    if (res != ESP_OK) {
        ESP_LOGE(LTAG, "Failed to set scan params, err %x", res);
        return Result::Fail;
    } else {
        return Result::Ok;
    }
}
