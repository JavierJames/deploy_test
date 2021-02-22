#pragma once

#include "HAL.hpp"

#include "ota/BTC_ESPUpdate.hpp"
#include "ota/BTC_PICUpdate.hpp"

#include "ble/profiles/ble_prof_nus.h"

/**
 * BLE NUS packet handler.
 */
class BTC_Parser {
   public:
    /**
     * Class constructor.
     *
     * @param _blueman pointer to the interface used to access they PIC module.
     */
    BTC_Parser(BTC_PICUpdate& _pic_ota, BTC_ESPUpdate& _esp_ota);

    /**
     * Parses the given pkt.
     *
     * @param pkt packet to be parsed.
     */
    void parsePkt(spp_pkt_t* pkt);

   private:
    BTC_PICUpdate& pic_ota;
    BTC_ESPUpdate& esp_ota;
    BTC_OTAUpdate* active_ota;
};


