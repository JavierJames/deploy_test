#include "UART_HAL.hpp"
#include "gmock/gmock.h"

class UART_Mock : public UART_HAL {
   public:
    MOCK_METHOD1(begin, void(int baudrate));
    MOCK_METHOD2(read, int(void* data, size_t len));
    MOCK_METHOD2(write, int(const void* data, size_t len));
};
