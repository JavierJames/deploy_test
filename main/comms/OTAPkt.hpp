#pragma once

#include <cstdint>
#include "SerialPkt.hpp"

/**
 * Status of ESP OTA.
 */
enum class FwUpdtStatus : uint8_t {
    START = 0,
    COMPLETE,
    FAIL
};

/**
 * A NUS packet is used during the PIC OTA update to send raw bytes.
 */
class OTAPkt : public SerialPkt {
   public:
    void encode(uint8_t* data, size_t data_len);
};
