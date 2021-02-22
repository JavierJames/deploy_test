#include "dataset.hpp"

#include <cassert>

#include "helper.hpp"

using namespace Kien;

static const char LTAG[] = "UTA_DS";

#include "globals.hpp"
static SysInfo& sys = getSysInfo();

#define BEACON_BUFFER_LEN 32

/**
 * Internal buffer to store the received beacons.
 */
static Queue<ds_uta_t> _rx_queue(BEACON_BUFFER_LEN);

Queue<ds_uta_t>& Kien::ds_get_rx_queue() {
    return _rx_queue;
}

/**
 * Parse the BLE beacon.
 *
 * @param ds dataset to store the UTA data
 * @param beacon received beacon
 * @param rssi BLE RF signal strength from the received packet beacon
 */
void Kien::ds_parse_beacon(ds_uta_t& ds,
                           const Kien::altbeacon_t& beacon,
                           int8_t rssi) {
    ds.tof = beacon.tof;
    ds.room_id = beacon.room_id;
    ds.rssi = rssi;
}

/**
 * Parse the BLE scan response.
 *
 * @param ds dataset to store the UTA data
 * @param scanres received scan response
 */
void Kien::ds_parse_scanres(ds_uta_t& ds,
                            const Kien::scan_response_t& scanres) {
    ds.spk_tx_id = {ascii2byte(scanres.name_id[0], scanres.name_id[1]),
                    ascii2byte(scanres.name_id[2], scanres.name_id[3]),
                    ascii2byte(scanres.name_id[4], scanres.name_id[5])};
    sys.getSpkId(ds.spk_rx_id);
}

/**
 * Serialize the dataset into a char array.
 *
 * @param ds dataset to be serialized
 * @param data pointer to the data array to hold the serialized result
 * @param data_len length of the data array
 *
 * @returns resulting size of the serialized string
 */
uint8_t Kien::ds_serialize(const ds_uta_t& ds, char* data, uint8_t data_len) {
    assert(data);
    int res =
        snprintf(data, data_len, "%02X%02X%02X,%02X%02X%02X,%02X,%04X,%02X",
                 ds.spk_rx_id.at(0), ds.spk_rx_id.at(1), ds.spk_rx_id.at(2),
                 ds.spk_tx_id.at(0), ds.spk_tx_id.at(1), ds.spk_tx_id.at(2),
                 ds.room_id, ds.tof, static_cast<uint8_t>(ds.rssi));

    assert(res > 0 && res < data_len);
    return res;
}
