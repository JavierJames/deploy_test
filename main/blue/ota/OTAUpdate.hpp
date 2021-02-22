#pragma once

#include <cstddef>
#include "blue/ble/profiles/ble_nus_cmd.hpp"

#ifdef TDD
#define OTA_BURST_NUM_PKTS 10
#else
#define OTA_BURST_NUM_PKTS 100
#endif

typedef struct {
    // Adv flags
    uint8_t device_type;
    uint8_t hw_rev;
    uint8_t fw_ver;
    uint32_t fw_len;
    uint16_t crc;

    int32_t _rsv;
} __attribute__((packed)) ota_init_pkt_t;

/**
 * Class to handle the speaker Over The Air updates.
 */
class OTAUpdate {
   public:
    OTAUpdate();

    /**
     * Prepare speaker to begin update (stops audio, disconnects A2DP link...).
     */
    virtual void start(const ota_init_pkt_t& init);

    /**
     * Handles the update commands once the process has started.
     */
    virtual void processPkt(nus_pkt_t& pkt);

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
    nus_pkt_t nuspkt;

   private:
    nus_pkt_t crc_pkt;
    uint16_t crc_aux;
    uint16_t crc_total;
};
