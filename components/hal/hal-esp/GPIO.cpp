#include "GPIO.hpp"
#include "driver/gpio.h"

GPIO_ESP::GPIO_ESP() {
    this->open();
}

void GPIO_ESP::open() {
    gpio_set_direction(DEBUG_LED_PIN, GPIO_MODE_OUTPUT);

    gpio_set_direction(STAT1_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(STAT1_PIN, GPIO_PULLUP_ONLY);

    gpio_set_direction(STAT2_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(STAT2_PIN, GPIO_PULLUP_ONLY);

    // Comms PIC-ESP
    gpio_set_direction(PIC_ESP_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIC_ESP_PIN, GPIO_PULLUP_ONLY);
    gpio_wakeup_enable(PIC_ESP_PIN, GPIO_INTR_HIGH_LEVEL);
    gpio_set_direction(ESP_PIC_PIN, GPIO_MODE_OUTPUT);
    this->clear(ESP_PIC_PIN);
}

ePinState GPIO_ESP::get(uint8_t pin) {
    return (gpio_get_level((gpio_num_t)pin) ? ePinHigh : ePinLow);
}

void GPIO_ESP::set(uint8_t pin) {
    gpio_set_level((gpio_num_t)pin, 1);
}

void GPIO_ESP::set(uint8_t pin, uint8_t level) {
    gpio_set_level((gpio_num_t)pin, (uint32_t)level);
}

void GPIO_ESP::clear(uint8_t pin) {
    gpio_set_level((gpio_num_t)pin, 0);
}
