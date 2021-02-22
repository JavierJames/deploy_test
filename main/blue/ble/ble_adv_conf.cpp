#include "ble_adv_conf.hpp"

#include "HAL.hpp"

#include <cassert>
#include <cstring>

#define PIC_UNKNOWN_VERSION "pfff"
#define NRF_UNKNOWN_VERSION "nfff"
#define BLE_REF_RSSI_1M 0

static const char LTAG[] = "BLE_ADV";

#define BLE_ADV_FLAG_LIMIT_DISC (0x01 << 0)
#define BLE_ADV_FLAG_GEN_DISC (0x01 << 1)
#define BLE_ADV_FLAG_BREDR_NOT_SPT (0x01 << 2)
#define BLE_ADV_FLAG_DMT_CONTROLLER_SPT (0x01 << 3)
#define BLE_ADV_FLAG_DMT_HOST_SPT (0x01 << 4)
#define BLE_ADV_FLAG_NON_LIMIT_DISC (0x00)

void Kien::ble_adv_altbeacon_default(Kien::altbeacon_t& beacon) {
    memset(&beacon, 0, sizeof(beacon));

    beacon.flags_len = 0x02;
    beacon.flags_type = 0x01;  // flag data type (from BLE spec)
    beacon.flags = (BLE_ADV_FLAG_GEN_DISC | BLE_ADV_FLAG_BREDR_NOT_SPT);
    beacon.length = 0x1B;
    beacon.type = 0xFF;  // manufacturer data type (from BLE spec)
    beacon.company_id = 0xFFFF;
    beacon.beacon_code = 0xACBE;  // Value given by the standard
    beacon.ref_rssi = BLE_REF_RSSI_1M;
    memcpy(beacon.uuid, Kien::ble_uuid, UUID_LEN_128);
    beacon.battery_info = 0xFF;
    beacon.room_id = 0xFF;
    beacon.tof = 0xFF;
}

void Kien::ble_adv_scanres_default(Kien::scan_response_t& scanres) {
    memset(&scanres, 0, sizeof(scanres));

    scanres.man_length = 12;
    scanres.man_type = 0xFF;  // manufacturer data type (from BLE spec)
    memcpy(scanres.pic_ver, PIC_UNKNOWN_VERSION, 4);
    memcpy(scanres.nrf_ver, NRF_UNKNOWN_VERSION, 4);
    memset(scanres.coco, 0, 3);
    scanres.name_len = sizeof(scanres.name_raw) + 1;
    scanres.name_type = 0x09;  // Complete local name (from BLE spec)
    memcpy(scanres.name_raw, "kienSPK_000000", sizeof(scanres.name_raw));
}

void Kien::ble_adv_config_altbeacon_battery(Kien::altbeacon_t& beacon,
                                            uint8_t battery,
                                            bool charging) {
    if (battery > 100)
        LOGW(LTAG, "Battery capacity higher than 100%% (%i%%)", battery);

    if (charging) {
        beacon.battery_info = battery | 0x80;
    } else {
        beacon.battery_info = battery;
    }
}

void Kien::ble_adv_config_scanres_coco(Kien::scan_response_t& scanres,
                                       std::array<uint8_t, 3>& coco) {
    memcpy(scanres.coco, coco.data(), coco.size());
}

void Kien::ble_adv_config_scanres_ver(Kien::scan_response_t& scanres,
                                      const std::array<char, 3>& pic,
                                      const std::array<char, 3>& nrf) {
    scanres.pic_ver[0] = 'p';
    scanres.nrf_ver[0] = 'n';
    memcpy(scanres.pic_ver + 1, pic.data(), pic.size());
    memcpy(scanres.nrf_ver + 1, nrf.data(), nrf.size());
}

void Kien::ble_adv_config_scanres_name(Kien::scan_response_t& scanres,
                                       const char* name,
                                       uint8_t len) {
    if (name == nullptr)
        return;

    if (len > sizeof(scanres.name_raw)) {
        LOGW(LTAG, "Name truncated");
        len = sizeof(scanres.name_raw);
    }

    scanres.name_len = len + 1;
    scanres.name_type = 0x09;  // Complete local name
    memcpy(scanres.name_raw, name, len);
}

void Kien::ble_adv_config_scanres_name(Kien::scan_response_t& scanres,
                                       bool is_sat,
                                       const std::array<uint8_t, 3>& id) {
    char name[sizeof(scanres.name_raw) + 1];  // Space for \0
    if (is_sat) {
        snprintf(name, sizeof(name), "%s%02X%02X%02X", Kien::SAT_NAME_PREFIX,
                 id[0], id[1], id[2]);
    } else {
        snprintf(name, sizeof(name), "%s%02X%02X%02X", Kien::SUB_NAME_PREFIX,
                 id[0], id[1], id[2]);
    }
    memcpy(scanres.name_raw, name, sizeof(scanres.name_raw));
    LOGI(LTAG, "BLE name: %s", name);
}

void Kien::ble_adv_parse_id(Kien::scan_response_t& scanres,
                            std::array<uint8_t, 3>& id) {
    char nibbles[] = "00";
    memcpy(nibbles, scanres.name_id, 2);
    id.at(0) = strtol(nibbles, NULL, 16);
    memcpy(nibbles, scanres.name_id + 2, 2);
    id.at(1) = strtol(nibbles, NULL, 16);
    memcpy(nibbles, scanres.name_id + 4, 2);
    id.at(2) = strtol(nibbles, NULL, 16);
}

bool Kien::ble_adv_is_altbeacon(uint8_t* raw, uint8_t len) {
    if (raw == nullptr || len != sizeof(Kien::altbeacon_t))
        return false;

    Kien::altbeacon_t* beacon = reinterpret_cast<Kien::altbeacon_t*>(raw);
    if (!memcmp(beacon->uuid, Kien::ble_uuid, UUID_LEN_128)) {
        return true;
    } else {
        return false;
    }
}
