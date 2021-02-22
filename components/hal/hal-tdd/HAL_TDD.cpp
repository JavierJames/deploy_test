#include "HAL_TDD.hpp"
#include "ADC.hpp"
#include "GPIO.hpp"
#include "I2C.hpp"
#include "I2S.hpp"
#include "UART.hpp"

static GPIO_Mock gpioDriver;
static UART_Mock uartPICDriver;
static I2S_Mock i2s_out;
static I2C_Mock i2cDriver;
static ADC1_Mock adc1Driver;

HAL_TDD::HAL_TDD()
    : HAL(&gpioDriver, &uartPICDriver, &i2s_out, &adc1Driver, &i2cDriver){};

void HAL_TDD::setup() {}

HAL& getHAL() {
    static HAL_TDD hal;
    return hal;
}
