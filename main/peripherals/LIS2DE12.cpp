#include "LIS2DE12.hpp"
#include "HAL.hpp"

#define I2C_ADDR 0x18

static const char LTAG[] = "LIS2DE12";

static HAL& hal = getHAL();

bool LIS2DE12::ping() {
    uint8_t reg = 0x0F;
    uint8_t data = 0;

    if (hal.i2c->transceive(I2C_ADDR, &reg, 1, &data, 1)) {
        LOGI(LTAG, "Accelerometer OK");
        return (data == 0x33);
    } else {
        LOGW(LTAG, "Accelerometer not responding");
        return false;
    }
}
