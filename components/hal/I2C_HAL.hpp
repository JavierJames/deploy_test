#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * I2C driver interface.
 */
class I2C_HAL {
   public:
    /**
     * Initialize I2C peripheral.
     */
    virtual void begin() = 0;

    /**
     * Start a write command.
     *
     * @param addr 7-bits slave address.
     * @param data pointer to the data to be written.
     * @param len lenght of the data to be written.
     */
    virtual bool write(uint8_t addr, uint8_t* data, size_t len) = 0;

    /**
     * Start a read command.
     *
     * @param addr 7-bits slave address.
     * @param data pointer to the data to be read.
     * @param len lenght of the data to be read.
     */
    virtual bool read(uint8_t addr, uint8_t* data, size_t len) = 0;

    /**
     * Start a write-read command.
     *
     * @param addr 7-bits slave address.
     * @param data_w pointer to the data to be written.
     * @param len_w lenght of the data to be written.
     * @param data_r pointer to the data to be read.
     * @param len_r lenght of the data to be read.
     */
    virtual bool transceive(uint8_t addr,
                            uint8_t* data_w,
                            size_t len_w,
                            uint8_t* data_r,
                            size_t len_r) = 0;
};
