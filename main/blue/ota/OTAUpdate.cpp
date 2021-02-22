#include "OTAUpdate.hpp"

#include "HAL.hpp"
#include "globals.hpp"
static BluemanInterface& ble = getBlueman();

#include "system/crc.h"

static const char LTAG[] = "OTA";

OTAUpdate::OTAUpdate()
    : ota_in_progress(false),
      recv_len(0),
      total_len(0),
      recv_pkts(0),
      crc_aux(0x1D0F),
      crc_total(0) {}

void OTAUpdate::start(const ota_init_pkt_t& init) {

    isOtaBtClassic = false;
    
    LOGI(LTAG, "Device type 0x%02X", init.device_type);
    LOGI(LTAG, "HW rev 0x%02X", init.hw_rev);
    LOGI(LTAG, "FW rev 0x%02X", init.fw_ver);
    LOGI(LTAG, "FW len %i", init.fw_len);
    LOGI(LTAG, "CRC 0x%04X", init.crc);

    LOGI(LTAG, "Preparing speaker");
    if (ota_in_progress == true) {
        LOGE(LTAG, "OTA already in progress");
    } else if (init.fw_len == 0) {
        LOGE(LTAG, "OTA size not valid");
    } else {
        ota_in_progress = true;
        this->recv_len = 0;
        this->total_len = init.fw_len;
        this->recv_pkts = 0;
        this->crc_total = init.crc;
        this->crc_aux = 0x1D0F;
    }
}

void OTAUpdate::processPkt(nus_pkt_t& pkt) {
    this->recv_len += pkt.len - 1;  // Subtract CMD len
    LOGD(LTAG, "Received chunk (%i/%i)", this->recv_len, this->total_len);
    this->crc_aux = CRCCCITT(pkt.data + 1, pkt.len - 1, this->crc_aux);

    if (++this->recv_pkts == OTA_BURST_NUM_PKTS) {
        LOGD(LTAG, "Sending CRC 0x%04X", this->crc_aux);
        crc_pkt.data[0] = BLE_NUS_CMD_OTA_CHUNK_ACK;
        memcpy(crc_pkt.data + 1, &this->crc_aux, 2);
        crc_pkt.len = 3;
        this->recv_pkts = 0;

        ble.send(crc_pkt);
    }
}

void OTAUpdate::finish(bool success) {
    LOGI(LTAG, "Restoring speaker");
    nuspkt.data[0] = BLE_NUS_CMD_OTA_RES;
    if (success && (this->crc_aux == this->crc_total)) {
        nuspkt.data[1] = 0;
    } else {
        nuspkt.data[1] = 1;
    }
    nuspkt.len = 2;
    ble.send(nuspkt);
    ota_in_progress = false;
}

void OTAUpdate::abort() {
    LOGE(LTAG, "Stopping update");
    ota_in_progress = false;
}
