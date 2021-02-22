#pragma once

#include <array>
#include <cstdint>

#define UUID_LEN_128 16

namespace Kien {

typedef struct {
    // Adv flags
    uint8_t flags_len;   // must be 0x02
    uint8_t flags_type;  // must be 0x01
    uint8_t flags;

    // Kien custom data
    uint8_t length;
    uint8_t type;
    uint16_t company_id;
    uint16_t beacon_code;
    uint8_t uuid[UUID_LEN_128];
    uint8_t rsv;  // reserved for future usage
    uint8_t room_id;
    uint16_t tof;  // time_of_flight
    int8_t ref_rssi;
    uint8_t battery_info;
} __attribute__((packed)) altbeacon_t;

typedef struct {
    // Kien custom data
    uint8_t man_length;
    uint8_t man_type;  // must be 0xFF
    char pic_ver[4];
    char nrf_ver[4];
    uint8_t coco[3];

    // BLE name
    uint8_t name_len;
    uint8_t name_type;  // must be 0x09
    // The format of the name is name_prefix + name_id = name_raw
    // (e.g. "kienSAT_" + "A1B3C3" = kienSAT_A1B3C3)
    union {
        char name_raw[14];
        struct {
            char name_prefix[8];
            char name_id[6];
        };
    };
} __attribute__((packed)) scan_response_t;

const char SAT_NAME_PREFIX[9] = "kienSAT_";
const char SUB_NAME_PREFIX[9] = "kienSUB_";
const uint8_t ble_uuid[UUID_LEN_128] = {
    // f380fe3c-c78e-4192-9d9f-19f29762fbf8
    0xf3, 0x80, 0xfe, 0x3c, 0xc7, 0x8e, 0x41, 0x92,
    0x9d, 0x9f, 0x19, 0xf2, 0x97, 0x62, 0xfb, 0xf8};
/**
 * Set beacon fields to their default values.
 *
 * @param beacon struct to be modified.
 */
void ble_adv_altbeacon_default(altbeacon_t& beacon);

/**
 * Set scan response fields to their default values.
 *
 * @param scanres struct to be modified.
 */
void ble_adv_scanres_default(scan_response_t& scanres);

/**
 * Update the battery status in the given altbeacon struct.
 *
 * @param beacon struct to be modified.
 * @param battery percentage indicating battery capacity (0-100). Values
 * outside this range are accepted but a warning is raised.
 * @param charging true when charging, false if not.
 */
void ble_adv_config_altbeacon_battery(altbeacon_t& beacon,
                                      uint8_t battery,
                                      bool charging);

/**
 * Update the firmware version in the given scan response struct.
 *
 * @param scanres struct to be modified.
 * @param pic firmware version.
 * @param nrf firmware version.
 */
void ble_adv_config_scanres_ver(scan_response_t& scanres,
                                const std::array<char, 3>& pic,
                                const std::array<char, 3>& nrf);

/**
 * Update the CoCo in the given scan response struct.
 *
 * @param scanres struct to be modified.
 * @param CoCo new value.
 */
void ble_adv_config_scanres_coco(scan_response_t& scanres,
                                 std::array<uint8_t, 3>& coco);

/**
 * Update the speaker name in the given scan response struct.
 *
 * @param scanres struct to be modified.
 * @param name device name.
 * @param len size of the name in bytes.
 */
void ble_adv_config_scanres_name(scan_response_t& scanres,
                                 const char* name,
                                 uint8_t len);

/**
 * Update the speaker name in the given scan response struct.
 *
 * @param scanres struct to be modified.
 * @param is_sat boolean indicating whether to use a sat or a sub name.
 * @param id id to be appended to the name.
 */
void ble_adv_config_scanres_name(scan_response_t& scanres,
                                 bool is_sat,
                                 const std::array<uint8_t, 3>& id);

/**
 * Parse the ID from the speaker name.
 *
 * @param scanres struct to be parsed.
 * @param id cointainer to put the resulting id.
 */
void ble_adv_parse_id(scan_response_t& scanres, std::array<uint8_t, 3>& id);

/**
 * Check if the given array is a known Kien beacon.
 *
 * @returns true if the array corresponds to a Kien beacon, false otherwise.
 */
bool ble_adv_is_altbeacon(uint8_t* raw, uint8_t len);

}  // namespace Kien
