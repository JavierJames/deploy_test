#include "I2C.hpp"

#include "freertos/FreeRTOS.h"

#include "driver/i2c.h"
#include "pins.h"

I2C_ESP::I2C_ESP(int _port) : port(_port) {}

void I2C_ESP::begin() {
    i2c_config_t i2c_config = {};
    i2c_config.mode = I2C_MODE_MASTER;
    i2c_config.sda_io_num = I2C_SDA_PIN;
    i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.scl_io_num = I2C_SCL_PIN;
    i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.master.clk_speed = 100000;
    i2c_param_config((i2c_port_t)this->port, &i2c_config);

    if(!is_installed)
        ESP_ERROR_CHECK(i2c_driver_install((i2c_port_t)this->port, i2c_config.mode, 0, 0, 0));

    is_installed = true;
}

bool I2C_ESP::write(uint8_t addr, uint8_t* data, size_t len) {
    if (len == 0) {
        return false;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin((i2c_port_t)this->port, cmd,
                                   1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? true : false;
}

bool I2C_ESP::read(uint8_t addr, uint8_t* data, size_t len) {
    if (len == 0) {
        return false;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin((i2c_port_t)this->port, cmd,
                                   1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? true : false;
}

bool I2C_ESP::transceive(uint8_t addr,
                         uint8_t* data_w,
                         size_t len_w,
                         uint8_t* data_r,
                         size_t len_r) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data_w, len_w, true);

    i2c_master_start(cmd);  // start repeat
    i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_READ, true);
    if (len_r > 1) {
        i2c_master_read(cmd, data_r, len_r - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data_r + len_r - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin((i2c_port_t)this->port, cmd,
                                   1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? true : false;
}
