#pragma once

#include <stdint.h>
#include "ADC_HAL.hpp"

typedef struct sADCConfiguration {
} sADCConfiguration;

typedef struct sADC1Configuration {
} sADC1Configuration;

/**
 * ADC driver for ESP32.
 */
class ADC_Mock : virtual public ADC_HAL {
   public:
   protected:
    MOCK_METHOD2(raw_to_voltage,
                 uint32_t(uint32_t adc_reading, sADCConfiguration* config_));
    MOCK_METHOD1(characterize, void(sADCConfiguration* config_));
};

/**
 * ADC1 driver for ESP32.
 */
class ADC1_Mock : public ADC1_HAL, protected ADC_Mock {
   public:
    MOCK_METHOD0(begin, void(void));
    MOCK_METHOD0(get_raw, int(void));
    MOCK_METHOD1(raw_to_voltage, uint32_t(uint32_t adc_reading));
};
