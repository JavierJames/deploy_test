#pragma once

#include <stddef.h>
#include <stdint.h>
#include "UART_HAL.hpp"

/**
 * UART driver for ESP32.
 */
class UART_ESP : public UART_HAL {
   public:
    /**
     * Defines UART configuration.
     */
    explicit UART_ESP(int port);

    void begin(int baudrate) override;
    int read(void* data, size_t len) override;
    int write(const void* data, size_t len) override;

   private:
    int port;
};
