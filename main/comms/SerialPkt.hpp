#pragma once

#include <cstddef>
#include <cstring>

#define SERIAL_RAW_MAX_LEN 50

/**
 * Parent class that can be sent to the PIC using the PICStream.
 */
class SerialPkt {
   public:
    SerialPkt() : rawLen(0) { memset(this->raw, 0, sizeof(this->raw)); };

    /**
     * Raw bytes that will be sent to the PIC.
     */
    char raw[SERIAL_RAW_MAX_LEN];

    /**
     * Number of bytes that will be read from SerialPkt::raw and sent to the
     * PIC.
     */
    size_t rawLen;
};
