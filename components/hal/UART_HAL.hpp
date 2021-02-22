#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * UART driver interface.
 */
class UART_HAL {
   public:
    /**
     * Starts running UART at the given baudrate.
     *
     * @param baudrate port baudrate.
     */
    virtual void begin(int baudrate) = 0;

    /**
     * Read len bytes and save them to data.
     *
     * @param data pointer to array to save the data.
     * @param len number of bytes to be read.
     *
     * @return number of bytes read or -1 if error.
     */
    virtual int read(void* data, size_t len) = 0;

    /**
     * Write len bytes from data.
     *
     * @param data pointer to array containing data to be written.
     * @param len number of bytes to be written.
     *
     * @return number of bytes written or -1 if error.
     */
    virtual int write(const void* data, size_t len) = 0;
};
