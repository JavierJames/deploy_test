#pragma once

#include <stdint.h>

/**
 * I2S driver interface.
 */
class I2S_HAL {
   public:
    /**
     * Starts running I2S with the given clocks.
     *
     * @param sample_rate output data frequency.
     * @param sample_width number of bits per sample (8, 16, 32).
     */
    virtual void begin(int sample_rate, uint8_t sample_width) = 0;

    /**
     * Zeroes the I2S DMA buffer.
     */
    virtual void flush() = 0;

    /**
     * Uninitializes the I2S peripheral.
     */
    virtual void close() = 0;

    /**
     * Pause the I2S clocks.
     */
    virtual void pause() = 0;

    /**
     * Resume the I2S clocks.
     */
    virtual void resume() = 0;
};
