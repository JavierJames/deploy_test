#include "DebugLED.hpp"

static HAL& hal = getHAL();
static const char LTAG[] = "DLED";

void DebugLED::task() {
    int level = 0;
    LOGI(LTAG, "Blinking LED every %i ms", this->blinkPeriod);
    for (;;) {
        hal.gpio->set(DEBUG_LED_PIN, level);
        level = !level;
        Task::delay(this->blinkPeriod);
    }
    this->remove();
}

void DebugLED::blinkStart(uint32_t period) {
    this->blinkPeriod = period;
    this->create("debug_led", 2048, 10);
}

void DebugLED::blinkStop() {
    this->remove();
}
