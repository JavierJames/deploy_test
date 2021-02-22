#include "ble_gatt.hpp"

#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_system.h"

#include "ble_peripheral.hpp"
#include "profiles/ble_prof_nus.h"

static const char LTAG[] = "BLE_GATT";

enum {
    KIEN_PROFILE_NUS_APP_ID,

    KIEN_PROFILE_NB  // Total number of profiles. This shall be the last item.
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    // uint16_t app_id;
    // uint16_t conn_id;
    // uint16_t service_handle;
    // esp_gatt_srvc_id_t service_id;
    // uint16_t char_handle;
    // esp_bt_uuid_t char_uuid;
    // esp_gatt_perm_t perm;
    // esp_gatt_char_prop_t property;
    // uint16_t descr_handle;
    // esp_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the
 * gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst _kien_profile_table[KIEN_PROFILE_NB] = {
    [KIEN_PROFILE_NUS_APP_ID] =
        {
            .gatts_cb = nus_evt_handlr,
            .gatts_if = ESP_GATT_IF_NONE,
        },
};

static void _gatts_event_handler(esp_gatts_cb_event_t event,
                                 esp_gatt_if_t gatts_if,
                                 esp_ble_gatts_cb_param_t* param) {
    ESP_LOGD(LTAG, "EVT %d, gatts if %d", event, gatts_if);
    if (param == nullptr) {
        ESP_LOGE(LTAG, "Callback *param is NULL");
        return;
    }

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            _kien_profile_table[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(LTAG, "Reg app failed, app_id %04X, status %d",
                     param->reg.app_id, param->reg.status);
            return;
        }
    } else if (event == ESP_GATTS_DISCONNECT_EVT) {
        Kien::ble_peripheral_start_adv();
    }

    do {
        int idx;
        for (idx = 0; idx < KIEN_PROFILE_NB; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not
                                                   specify a certain
                                                   gatt_if, need to call
                                                   every profile cb function
                                                 */
                gatts_if == _kien_profile_table[idx].gatts_if) {
                if (_kien_profile_table[idx].gatts_cb) {
                    _kien_profile_table[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void Kien::ble_gatt_init() {
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(_gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(KIEN_PROFILE_NUS_APP_ID));
}
