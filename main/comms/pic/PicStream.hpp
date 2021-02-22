#pragma once

#include "HAL.hpp"

#include "PicParser.hpp"

/**
 * Manages the serial communications between PIC and ESP.
 */
class PicStream : public Task {
   public:
    /**
     * Class constructor.
     *
     * @param _parser class to parse and respond to the received commands.
     */
    explicit PicStream(PicParser& _parser);

    /**
     * Queue of packets to be sent to the PIC.
     */
    Queue<SerialPkt> txqueue;

   private:
    void task() override;

    /**
     * Pointer to the packet parser.
     */
    PicParser& parser;

    /**
     * Object used to dequeue the received bytes.
     */
    NMEAPkt pktRx;

    /**
     * Internal buffer to save the received bytes until a '\0' is received.
     * It limits the max size of the packet.
     */
    char rxBuff[2 * SERIAL_RAW_MAX_LEN];

    /**
     * Position of the last received byte in PicParser::rxBuff.
     */
    size_t rxBuffLastPos;
};
