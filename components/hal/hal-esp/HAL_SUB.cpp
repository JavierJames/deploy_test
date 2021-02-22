#include "HAL_SUB.hpp"
#include "GPIO.hpp"
#include "UART.hpp"

static GPIO_ESP gpioDriver;
static UART_ESP uartPICDriver(1);

HAL_SUB::HAL_SUB() : HAL(&gpioDriver, &uartPICDriver, nullptr, nullptr, nullptr){};

void HAL_SUB::setup() {
    uartPIC->begin(115200);
}

HAL& getHAL() {
    static HAL_SUB hal;
    return hal;
}
