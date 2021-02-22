#include "SysInfo.hpp"

#include <cstring>

static const char LTAG[] = "SYSINFO";

SysInfo::SysInfo(Mutex* _mtx)
    : picFwName{},
      picFwNameLen(0),
      picFwVer(PIC_FW_VERSION_ERROR),
      espFwName(ESP_FW_NAME),
      espFwNameLen(sizeof(ESP_FW_NAME) - 1),  // Do not count '\0'
      espFwVer(ESP_FW_VERSION),
      spkMode(SpkMode::UNKNOWN),
      spkId(SPK_ID_DEFAULT),
      mtx(_mtx){};

size_t SysInfo::getPicFwName(char* name) {
        memcpy(name, this->picFwName, this->picFwNameLen);
        return this->picFwNameLen;
}

bool SysInfo::setPicFwName(const char* name, size_t len) {
        memcpy(this->picFwName, name, len);
        this->picFwNameLen = len;
        return true;
}

bool SysInfo::getPicFwVersion(std::array<char, FW_VER_LEN>& ver) {
        ver = this->picFwVer;
        return true;
}

bool SysInfo::setPicFwVersion(const std::array<char, FW_VER_LEN>& ver) {
        this->picFwVer = ver;
        return true;
}

size_t SysInfo::getEspFwName(char* name) {
    memcpy(name, espFwName, espFwNameLen);
    return espFwNameLen;
}

void SysInfo::getEspFwVersion(std::array<char, FW_VER_LEN>& ver) {
    ver = espFwVer;
}

bool SysInfo::getCurrentSpkMode(SpkMode* mode) {
        *mode = this->spkMode;
        return true;
}

bool SysInfo::setCurrentSpkMode(SpkMode mode) {
        this->spkMode = mode;
        return true;
}

bool SysInfo::getSpkId(std::array<uint8_t, 3>& id) {
        id = this->spkId;
        return true;
}

bool SysInfo::setSpkId(const std::array<uint8_t, 3>& id) {
        this->spkId = id;
        return true;
}
