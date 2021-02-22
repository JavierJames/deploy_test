#pragma once

#include "HAL.hpp"

#include "comms/GroupInfo.hpp"
#include "comms/NMEAPkt.hpp"

/**
 * PIC packet handler.
 */
class PicParser {
   public:
    /**
     * Class constructor.
     *
     * @param _newMode pointer to a semaphore to trigger when a new mode is
     * detected.
     */
    explicit PicParser(BinarySemaphore& _newMode);

    void (*unlock_mut_group)(void);

    /**
     * Parses the given pkt.
     *
     * @param pkt packet to be parsed.
     * @param txqueue queue to use in case the packet has to respond with
     * another packet.
     */
    void parsePkt(NMEAPkt& pkt, Queue<SerialPkt>* txqueue);


    GroupInfo group;

   private:
    BinarySemaphore& newMode;
};
