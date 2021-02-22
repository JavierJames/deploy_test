#include "I2S.hpp"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/i2s.h"

#include "pins.h"

extern bool i2s_is_paused; 

I2S_ESP::I2S_ESP(int _port) : port(_port) {}

void I2S_ESP::begin(int sample_rate, uint8_t sample_width) {
    i2s_config_t i2s_config = {};
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);  // Only TX
    i2s_config.sample_rate = sample_rate;
    i2s_config.bits_per_sample = (i2s_bits_per_sample_t)sample_width;
    i2s_config.channel_format =
        (i2s_channel_fmt_t)I2S_CHANNEL_FMT_RIGHT_LEFT;  // 2-channels
    i2s_config.communication_format =
        (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
    i2s_config.dma_buf_count = 128;
    i2s_config.dma_buf_len = 64;
    i2s_config.intr_alloc_flags = 0;  // default interrupt priority
    i2s_config.use_apll = true;
    i2s_config.tx_desc_auto_clear = true;

    ESP_ERROR_CHECK(
        i2s_driver_install((i2s_port_t)this->port, &i2s_config, 0, NULL));

    i2s_pin_config_t pin_config = {};
    pin_config.bck_io_num = I2S_OUT_SCLK_PIN;
    pin_config.ws_io_num = I2S_OUT_LRCLK_PIN;
    pin_config.data_out_num = I2S_OUT_DATA_PIN;
    pin_config.data_in_num = I2S_PIN_NO_CHANGE;  // Not used

    ESP_ERROR_CHECK(i2s_set_pin((i2s_port_t)this->port, &pin_config));

    this->available = true;
}

void I2S_ESP::flush() {
    if (this->available == false)
        return;

    i2s_zero_dma_buffer((i2s_port_t)this->port);
}

void I2S_ESP::pause() {
    if (this->available == false)
        return;

    this->flush();

    i2s_is_paused = true;

    gpio_set_direction(I2S_OUT_LRCLK_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(I2S_OUT_DATA_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(I2S_OUT_SCLK_PIN, GPIO_MODE_INPUT);

}

void I2S_ESP::resume() {
    if (this->available == false)
        return;

    gpio_set_direction(I2S_OUT_LRCLK_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(I2S_OUT_DATA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(I2S_OUT_SCLK_PIN, GPIO_MODE_OUTPUT);

    i2s_pin_config_t pin_config = {};
    pin_config.bck_io_num = I2S_OUT_SCLK_PIN;
    pin_config.ws_io_num = I2S_OUT_LRCLK_PIN;
    pin_config.data_out_num = I2S_OUT_DATA_PIN;
    pin_config.data_in_num = I2S_PIN_NO_CHANGE;  // Not used
    ESP_ERROR_CHECK(i2s_set_pin((i2s_port_t)this->port, &pin_config));

    esp_err_t err = i2s_start((i2s_port_t)this->port);
    if(err)
        ESP_LOGE("I2S", "I2S start failed with error (%s)", esp_err_to_name(err));

    i2s_is_paused = false;
}

void I2S_ESP::close() {
    if (this->available == false)
        return;

    this->available = false;

    i2s_driver_uninstall((i2s_port_t)this->port);
    this->flush();

    gpio_set_direction(I2S_OUT_LRCLK_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(I2S_OUT_DATA_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(I2S_OUT_SCLK_PIN, GPIO_MODE_INPUT);
}
