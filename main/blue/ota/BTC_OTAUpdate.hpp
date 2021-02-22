#pragma once

#include <cstddef>
#include "blue/spp/btc_spp_cmd.hpp"

// #ifdef TDD
// #define BTC_OTA_BURST_NUM_PKTS 10
// #else
// #define BTC_OTA_BURST_NUM_PKTS 1
// #endif

typedef struct {
    // Adv flags
    uint8_t device_type;
    uint8_t hw_rev;
    uint8_t fw_ver;
    uint32_t fw_len;
    uint16_t crc;
    uint16_t chunksize;
    uint8_t burstNumPkts;
    int8_t _rsv;
} __attribute__((packed)) btc_ota_init_pkt_t;

/**
 * Class to handle the speaker Over The Air updates.
 */
class BTC_OTAUpdate {
   public:
    BTC_OTAUpdate();

    /**
     * Prepare speaker to begin update (stops audio, disconnects A2DP link...).
     */
    virtual void start(const btc_ota_init_pkt_t& init);

    /**
     * Handles the update commands once the process has started.
     */
    virtual void processPkt(spp_pkt_t& pkt);

    /**
     * Finalize update.
     */
    virtual void finish(bool success);

    /**
     * Cancel the update. Typically should notify the updater.
     */
    virtual void abort();



   protected:
    bool ota_in_progress;
    size_t recv_len;
    size_t total_len;
    size_t recv_pkts;
    // spp_pkt_t spp_pkt;
    spp_pkt_t *p_tx_pkt;
    uint16_t chunksize;
    uint8_t burstNumPkts;

    /**
     * Calculate the expected number of chunks that the firmware is divided into
    */
    uint16_t num_chunks();

    /**
     * Calculate the number of elements in the firmware last chunk
    */
    uint16_t num_elem_last_chunk();


   private:
    // spp_pkt_t crc_pkt;
    uint16_t crc_aux;
    uint16_t crc_total;
};
