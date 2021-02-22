#pragma once

#include <stdint.h>

struct sADCConfiguration;
struct sADC1Configuration;

/**
 * ADC driver interface.
 */
class ADC_HAL {
   protected:
    virtual uint32_t raw_to_voltage(uint32_t adc_reading,
                                    struct sADCConfiguration* config_) = 0;
    virtual void characterize(struct sADCConfiguration* config_) = 0;
};

/**
 * ADC1 driver interface.
 */
class ADC1_HAL : virtual protected ADC_HAL {
   public:
    virtual void begin(void) = 0;
    virtual int get_raw(void) = 0;
    virtual uint32_t raw_to_voltage(uint32_t adc_reading) = 0;
};
