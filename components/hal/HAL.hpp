#pragma once

// Run-time substitutions
#include "ADC_HAL.hpp"
#include "GPIO_HAL.hpp"
#include "I2C_HAL.hpp"
#include "I2S_HAL.hpp"
#include "System_HAL.hpp"
#include "UART_HAL.hpp"

// Link-time substitutions
#include "LOGS.h"
#include "Mutex.hpp"
#include "Queue.hpp"
#include "Semaphore.hpp"
#include "Task.hpp"
#include "pins.h"

/**
 * Class to access the peripherals in the Hardware Abstraction Layer.
 * It's a singleton, so it must be accessed through getHAL().
 */
class HAL {
   public:
    HAL(GPIO_HAL* _gpio,
        UART_HAL* _uartPIC,
        I2S_HAL* _i2sOut,
        ADC1_HAL* _adc,
        I2C_HAL* _i2c)
        : gpio(_gpio),
          uartPIC(_uartPIC),
          i2sOut(_i2sOut),
          adc1(_adc),
          i2c(_i2c){};
    GPIO_HAL* gpio;
    UART_HAL* uartPIC;
    I2S_HAL* i2sOut;
    ADC1_HAL* adc1;
    I2C_HAL* i2c;

    /**
     * Initializes the peripherals.
     */
    virtual void setup() = 0;
};

// Must be implemented by the concrete HALs.
HAL& getHAL();
