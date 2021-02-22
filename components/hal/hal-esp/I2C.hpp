#pragma once

#include "I2C_HAL.hpp"

/**
 * I2S driver for ESP32.
 */
class I2C_ESP : public I2C_HAL {
   public:
    explicit I2C_ESP(int port);

    void begin() override;
    bool write(uint8_t addr, uint8_t* data, size_t len) override;
    bool read(uint8_t addr, uint8_t* data, size_t len) override;
    bool transceive(uint8_t addr,
                    uint8_t* data_w,
                    size_t len_w,
                    uint8_t* data_r,
                    size_t len_r) override;
   private:
    int port;
    bool is_installed = false;
};
