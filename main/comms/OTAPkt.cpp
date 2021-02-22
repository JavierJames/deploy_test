#include "OTAPkt.hpp"

void OTAPkt::encode(uint8_t* data, size_t data_len) {
    memcpy(this->raw, data, data_len);
    this->rawLen = data_len;
}
