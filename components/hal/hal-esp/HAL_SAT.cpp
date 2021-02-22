#include "HAL_SAT.hpp"
#include "ADC.hpp"
#include "GPIO.hpp"
#include "I2C.hpp"
#include "I2S.hpp"
#include "UART.hpp"

static GPIO_ESP gpioDriver;
static UART_ESP uartPICDriver(1);
static I2S_ESP i2sOutDriver(0);
static I2C_ESP i2cDriver(0);
static ADC1_ESP ADC1Driver;

HAL_SAT::HAL_SAT()
    : HAL(&gpioDriver,
          &uartPICDriver,
          &i2sOutDriver,
          &ADC1Driver,
          &i2cDriver){};

void HAL_SAT::setup() {
    uartPIC->begin(115200);
    adc1->begin();
    i2c->begin();
}

HAL& getHAL() {
    static HAL_SAT hal;
    return hal;
}
