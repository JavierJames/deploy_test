#include "BTC_OTAUpdate.hpp"

#include "HAL.hpp"
#include "globals.hpp"
#include "a2dp/bt_app_av.h"
#include "btc_spp.h"
#include "crc.h"

static BTC_BluemanInterface& btc_blue = getBTCBlueman();


static const char LTAG[] = "BTC-OTA";

static spp_pkt_t tx_pkt;

BTC_OTAUpdate::BTC_OTAUpdate()
    : ota_in_progress(false),
      recv_len(0),
      total_len(0),
      recv_pkts(0),
      crc_aux(0x1D0F),
      crc_total(0) {

      p_tx_pkt =   &tx_pkt;

      }

void BTC_OTAUpdate::start(const btc_ota_init_pkt_t& init) {

    isOtaBtClassic = true;

    bt_a2d_close();
    taskYIELD();

    LOGI(LTAG, "Device type 0x%02X", init.device_type);
    LOGI(LTAG, "HW rev 0x%02X", init.hw_rev);
    LOGI(LTAG, "FW rev 0x%02X", init.fw_ver);
    LOGI(LTAG, "FW len %i", init.fw_len);
    LOGI(LTAG, "FW chunksize %i ", init.chunksize);
    LOGI(LTAG, "FW iterator %i ", init.burstNumPkts);
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
        this->chunksize = init.chunksize;
        this->burstNumPkts = init.burstNumPkts;
        this->crc_total = init.crc;
        this->crc_aux = 0x1D0F;
        btc_spp_init_crc();
    }
}

void BTC_OTAUpdate::processPkt(spp_pkt_t& pkt) {
    this->recv_len += pkt.len - 1;  // Subtract CMD len
    LOGD(LTAG, "Received chunk (%i/%i)", this->recv_len, this->total_len);
    this->crc_aux = btc_spp_crc(); // CRCCCITT(pkt.data + 1, pkt.len - 1, this->crc_aux);

    LOGD(LTAG, "recv_pkts %i", this->recv_pkts+1);
    if (++this->recv_pkts == this->burstNumPkts) { // BTC_OTA_BURST_NUM_PKTS) {
        LOGD(LTAG, "Received chunks (%i/%i). Sending CRC 0x%04X", this->recv_len, this->total_len, this->crc_aux);
        p_tx_pkt->data[0] = BTC_SPP_CMD_OTA_CHUNK_ACK;
        memcpy(p_tx_pkt->data + 1, &this->crc_aux, 2);
        p_tx_pkt->len = 3;
        this->recv_pkts = 0;

        btc_blue.send(*p_tx_pkt);
    }
}

void BTC_OTAUpdate::finish(bool success) {
    LOGI(LTAG, "Restoring speaker");
    p_tx_pkt->data[0] = BTC_SPP_CMD_OTA_RES;
    if (success && (this->crc_aux == this->crc_total)) {
        p_tx_pkt->data[1] = 0;
    } else {
        p_tx_pkt->data[1] = 1;
    }
    p_tx_pkt->len = 2;
    btc_blue.send(*p_tx_pkt);
    ota_in_progress = false;
}

void BTC_OTAUpdate::abort() {
    LOGE(LTAG, "Stopping update");
    ota_in_progress = false;
}


uint16_t BTC_OTAUpdate::num_chunks(){


    uint16_t quotient = this->total_len/this->chunksize;
    uint16_t remainder = this->total_len % this->chunksize;

    uint16_t num_chunks = quotient;

    //add 1 if there is a remainder
    if(remainder) num_chunks++;

    LOGD(LTAG, "Number of chunks: %d ", num_chunks);
    return num_chunks;
}


uint16_t BTC_OTAUpdate::num_elem_last_chunk(){

    uint16_t remainder = this->total_len % this->chunksize;
    LOGD(LTAG, "Number of elements in last chunk: %d ",remainder);
    return remainder;
}
