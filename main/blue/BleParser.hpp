#pragma once

#include "HAL.hpp"

#include "ota/ESPUpdate.hpp"
#include "ota/PICUpdate.hpp"

#include "ble/profiles/ble_prof_nus.h"

/**
 * BLE NUS packet handler.
 */
class BleParser {
   public:
    /**
     * Class constructor.
     *
     * @param _blueman pointer to the interface used to access they PIC module.
     */
    BleParser(PICUpdate& _pic_ota, ESPUpdate& _esp_ota);

    /**
     * Parses the given pkt.
     *
     * @param pkt packet to be parsed.
     * @param txqueue queue to use in case the packet has to respond with
     * another packet.
     */
    void parsePkt(nus_pkt_t* pkt, Queue<nus_pkt_t>& txqueue);

   private:
    PICUpdate& pic_ota;
    ESPUpdate& esp_ota;
    OTAUpdate* active_ota;
};
