#ifndef __PINS_H__
#define __PINS_H__

#include "driver/gpio.h"

#define PROTO_V2_PINS

#ifdef PROTO_V2_PINS

// TODO is not defined: map to unused pin ?
#define DEBUG_LED_PIN (GPIO_NUM_2)

// ADC1 channel 7 is GPIO35
#define BATMAN_ADC_CHANNEL (ADC_CHANNEL_7)

#define STAT1_PIN (GPIO_NUM_19)
#define STAT2_PIN (GPIO_NUM_26)

#define UTX1_PIN (GPIO_NUM_5)
#define URX1_PIN (GPIO_NUM_21)

#define I2S_OUT_DATA_PIN (GPIO_NUM_18)
#define I2S_OUT_SCLK_PIN (GPIO_NUM_27)
#define I2S_OUT_LRCLK_PIN (GPIO_NUM_25)

#define ESP_PIC_PIN (GPIO_NUM_4)
#define PIC_ESP_PIN (GPIO_NUM_15)

#define I2C_SDA_PIN (GPIO_NUM_22)
#define I2C_SCL_PIN (GPIO_NUM_23)

#else

#define DEBUG_LED_PIN (GPIO_NUM_2)

#define BATMAN_ADC_CHANNEL (ADC_CHANNEL_7)

#define STAT1_PIN (GPIO_NUM_17)
#define STAT2_PIN (GPIO_NUM_16)

#define UTX1_PIN (GPIO_NUM_4)
#define URX1_PIN (GPIO_NUM_5)

#define I2S_OUT_DATA_PIN (GPIO_NUM_13)
#define I2S_OUT_SCLK_PIN (GPIO_NUM_12)
#define I2S_OUT_LRCLK_PIN (GPIO_NUM_14)

#define ESP_PIC_PIN (GPIO_NUM_25)
#define PIC_ESP_PIN (GPIO_NUM_26)

#endif

#endif /* __PINS_H__ */
