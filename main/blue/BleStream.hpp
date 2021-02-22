#pragma once

#include "BleParser.hpp"

#include "HAL.hpp"
#include "ble/profiles/ble_prof_nus.h"

/**
 * Manages the BLE communications between APP and ESP.
 */
class BleStream : public Task {
   public:
    explicit BleStream(BleParser& _parser);

    /**
     * Queue for the NUS commands to be sent in Blueman::task().
     */
    Queue<nus_pkt_t> txQueue;

   private:
    void task() override;

    /**
     * Pointer to the packet parser.
     */
    BleParser& parser;

    /**
     * Queue for the NUS commands to be parsed in Blueman::task().
     */
    Queue<nus_pkt_t> rxQueue;
};
