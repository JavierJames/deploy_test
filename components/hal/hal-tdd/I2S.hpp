#include "I2S_HAL.hpp"
#include "gmock/gmock.h"

class I2S_Mock : public I2S_HAL {
   public:
    MOCK_METHOD2(begin, void(int sample_rate, uint8_t sample_width));
    MOCK_METHOD0(flush, void());
    MOCK_METHOD0(pause, void());
    MOCK_METHOD0(resume, void());
    MOCK_METHOD0(close, void());
};
