#include "NMEAPkt.hpp"

static const char LTAG[] = "NMEA";

NMEAPkt::NMEAPkt()
    : SerialPkt(),
      page(NMEAPkt::NMEAPktPages::PICCOMMS_INVALID_PAGE),
      chks(0),
      nextParamPtr(NULL){};

NMEAPkt::NMEAPkt(NMEAPkt::NMEAPktPages page, char* params) : NMEAPkt() {
    this->encode(page, params);
}

void NMEAPkt::encode(NMEAPkt::NMEAPktPages page, char* params) {
    this->page = page;
    if (params != NULL) {
        this->rawLen = snprintf(this->raw, SERIAL_RAW_MAX_LEN - 2, "$%02d,%s*",
                                static_cast<uint8_t>(page), params);
    } else {
        this->rawLen = snprintf(this->raw, SERIAL_RAW_MAX_LEN - 2, "$%02d*",
                                static_cast<uint8_t>(page));
    }
    this->chks = obtainChecksum();
    this->rawLen += snprintf(this->raw + this->rawLen, 3, "%02X", this->chks);
    this->rawLen++;  // count '\0'
    if (this->isRawBuffAvailable()) {
        LOGD(LTAG, "raw %s", this->raw);
    }
}

bool NMEAPkt::isRawBuffAvailable() {
    return (this->rawLen > 0) && (this->rawLen <= SERIAL_RAW_MAX_LEN);
}

uint8_t NMEAPkt::obtainChecksum() {
    uint8_t _chks = 0;
    for (size_t i = 1; i < this->rawLen; i++) {
        if (this->raw[i] == '*')
            return _chks;
        _chks ^= this->raw[i];
    }
    return _chks;
}

bool NMEAPkt::parseArray(char* data, size_t len) {
    // This function assumes the data packet is complete
    // by looking for '$' and '*' in the right place
    // Sample NMEA packet: $91,12,6a7*6B
    if (data[0] != '$' || (data[(len - 3) - 1] != '*')) {
        LOGW(LTAG, "Array incomplete");
        return false;
    }

    this->rawLen = len;
    if (!this->isRawBuffAvailable()) {
        return false;
    }

    memcpy(this->raw, data, len);
    this->nextParamPtr = this->raw;
    LOGD(LTAG, "raw %s", this->raw);

    this->chks = strtol(data + len - 3, NULL, 16);
    LOGD(LTAG, "Checkum: 0x%02X", this->chks);
    this->page = static_cast<NMEAPktPages>(strtol(data + 1, NULL, 10));
    LOGI(LTAG, "Received page: %d", static_cast<uint8_t>(this->page));

    if (this->obtainChecksum() == this->chks) {
        return true;
    } else {
        LOGE(LTAG, "Wrong checksum");
        return false;
    }
}

size_t NMEAPkt::nextParam(char** param) {
    if (this->nextParamPtr == NULL)  // No more parameters to process
        return 0;
    char* limit1 = strstr(this->nextParamPtr, ",");  // beginning of last param
    char* limit2 = NULL;                             // beginning of next param

    if (limit1 == NULL) {  // No more commands
        *param = NULL;
        this->nextParamPtr = NULL;
        return 0;
    } else {
        limit2 = strstr(++limit1, ",");
    }

    if (limit2 == NULL) {  // Last command in the packet
        limit2 = strstr(limit1, "*");
    }

    *param = limit1;
    this->nextParamPtr = limit2;
    return limit2 - limit1;
}
