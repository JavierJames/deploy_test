#pragma once

#include "I2S_HAL.hpp"

/**
 * I2S driver for ESP32.
 */
class I2S_ESP : public I2S_HAL {
   public:
    explicit I2S_ESP(int port);

    void begin(int sample_rate, uint8_t sample_width) override;
    void flush() override;
    void close() override;
    void pause() override;
    void resume() override;

   private:
    int port;
    bool available;
};
