#include "I2C_HAL.hpp"
#include "gmock/gmock.h"

class I2C_Mock : public I2C_HAL {
   public:
    MOCK_METHOD0(begin, void());
    MOCK_METHOD3(write, bool(uint8_t addr, uint8_t* data, size_t len));
    MOCK_METHOD3(read, bool(uint8_t addr, uint8_t* data, size_t len));
    MOCK_METHOD5(transceive,
                 bool(uint8_t addr,
                      uint8_t* data_w,
                      size_t len_w,
                      uint8_t* data_r,
                      size_t len_r));
};
