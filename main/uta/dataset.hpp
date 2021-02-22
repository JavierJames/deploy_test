#pragma once

#include <array>
#include <cstdint>

#include "blue/ble/ble_adv_conf.hpp"

#include "HAL.hpp"

namespace Kien {

typedef struct {
    uint16_t tof;  // time_of_flight
    uint8_t room_id;
    std::array<uint8_t, 3> spk_rx_id;
    std::array<uint8_t, 3> spk_tx_id;
    int8_t rssi;
} ds_uta_t;

Queue<ds_uta_t>& ds_get_rx_queue();

void ds_parse_beacon(ds_uta_t& ds,
                     const Kien::altbeacon_t& beacon,
                     int8_t rssi);

void ds_parse_scanres(ds_uta_t& ds, const Kien::scan_response_t& scanres);

uint8_t ds_serialize(const ds_uta_t& ds, char* ser, uint8_t data_len);

}  // namespace Kien
