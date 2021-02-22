#include "UART.hpp"

#include "driver/uart.h"
#include "pins.h"

UART_ESP::UART_ESP(int _port) : port(_port) {}

void UART_ESP::begin(int baudrate) {
    uart_config_t uart_config = {};
    uart_config.baud_rate = baudrate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    ESP_ERROR_CHECK(uart_param_config((uart_port_t)this->port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin((uart_port_t)this->port, UTX1_PIN, URX1_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    if(!uart_is_driver_installed((uart_port_t)this->port))
        ESP_ERROR_CHECK( uart_driver_install((uart_port_t)this->port, 1024, 1024, 10, NULL, 0));
}

int UART_ESP::read(void* data, size_t len) {
    return uart_read_bytes((uart_port_t)this->port, (uint8_t*)data, len,
                           20 / portTICK_RATE_MS);
}

int UART_ESP::write(const void* data, size_t len) {
    return uart_write_bytes((uart_port_t)this->port, (const char*)data, len);
}
