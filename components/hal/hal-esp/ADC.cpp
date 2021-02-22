#include "ADC.hpp"
#include "pins.h"

#include <string.h>
#include "driver/adc.h"

///// Defines

#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_UNIT ADC_UNIT_1
#define DEFAULT_VREF \
    1100  // TODO Use adc2_vref_to_gpio() to obtain a better estimate

///// ADC driver

void ADC_ESP::characterize(struct sADCConfiguration* config_) {
    assert(config_);

    // Characterize ADC
    memset(&config_->adc_char, 0, sizeof(esp_adc_cal_characteristics_t));
    (void)esp_adc_cal_characterize(config_->adc_num, config_->atten,
                                   config_->bit_width, config_->vref,
                                   &config_->adc_char);
}

uint32_t ADC_ESP::raw_to_voltage(uint32_t adc_reading,
                                 struct sADCConfiguration* config_) {
    assert(config_);

    return esp_adc_cal_raw_to_voltage(adc_reading, &config_->adc_char);
}

///// ADC1 driver

void ADC1_ESP::begin(void) {
    // reset configuration
    memset(&m_adc_config, 0, sizeof(m_adc_config));

    // set configuration
    m_adc_config.adc_gen_config.atten = ADC_ATTEN;
    m_adc_config.adc_gen_config.adc_num = ADC_UNIT;
    m_adc_config.adc_gen_config.bit_width = ADC_WIDTH_BIT_12;
    m_adc_config.adc_gen_config.vref = DEFAULT_VREF;
    m_adc_config.channel = (adc1_channel_t)BATMAN_ADC_CHANNEL;

    // configure adc1
    this->config();
}

void ADC1_ESP::config(void) {
    adc1_config_width(m_adc_config.adc_gen_config.bit_width);
    adc1_config_channel_atten(m_adc_config.channel,
                              m_adc_config.adc_gen_config.atten);

    this->characterize(&m_adc_config.adc_gen_config);
}

int ADC1_ESP::get_raw(void) {
    return adc1_get_raw(m_adc_config.channel);
}

uint32_t ADC1_ESP::raw_to_voltage(uint32_t adc_reading) {
    return ADC_ESP::raw_to_voltage(adc_reading, &m_adc_config.adc_gen_config);
}
