#include "SysInfo.hpp"

#include <cstring>

static const char LTAG[] = "SYSINFO";
static uint8_t _serial_number[SERIAL_NUM_LEN];
static bool _is_serial_number_loaded = false;


/*
  variable to keep trrack if to power speaker, if it is from the new 

*/

bool m_auto_pwr_off_enabled = true;


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

uint8_t* SysInfo::getSerialNumber()
{
    return _serial_number;
}

bool SysInfo::setSerialNumber(uint8_t* sn)
{
    if(sn == NULL){
        LOGW(LTAG,"Could not set serial number");
        _is_serial_number_loaded = false;
        return false;
    }

    memcpy(_serial_number, sn, SERIAL_NUM_LEN);
    _is_serial_number_loaded = true;
    return true;
}

bool SysInfo::isSerialNumberLoaded()
{
    return _is_serial_number_loaded;
}

size_t SysInfo::getPicFwName(char* name) {
    if (this->mtx->lock()) {
        memcpy(name, this->picFwName, this->picFwNameLen);
        LOGI(LTAG, "Getting PIC FW name: %s", this->picFwName);
        this->mtx->unlock();
        return this->picFwNameLen;
    } else {
        return 0;
    }
}

bool SysInfo::setPicFwName(const char* name, size_t len) {
    if (this->mtx->lock()) {
        memcpy(this->picFwName, name, len);
        this->picFwNameLen = len;
        LOGI(LTAG, "Setting PIC FW name: (%s, %i)", this->picFwName,
             this->picFwNameLen);
        this->mtx->unlock();
        return true;
    } else {
        return false;
    }
}

bool SysInfo::getPicFwVersion(std::array<char, FW_VER_LEN>& ver) {
    if (this->mtx->lock()) {
        ver = this->picFwVer;
        LOGI(LTAG, "Getting PIC FW ver: (%c%c%c)", this->picFwVer[0],
             this->picFwVer[1], this->picFwVer[2]);
        this->mtx->unlock();
        return true;
    } else {
        return false;
    }
}

bool SysInfo::setPicFwVersion(const std::array<char, FW_VER_LEN>& ver) {
    if (this->mtx->lock()) {
        this->picFwVer = ver;
        LOGI(LTAG, "Setting PIC FW ver: (%c%c%c)", this->picFwVer[0],
             this->picFwVer[1], this->picFwVer[2]);
        this->mtx->unlock();
        return true;
    } else {
        return false;
    }
}

size_t SysInfo::getEspFwName(char* name) {
    memcpy(name, espFwName, espFwNameLen);
    return espFwNameLen;
}

void SysInfo::getEspFwVersion(std::array<char, FW_VER_LEN>& ver) {
    ver = espFwVer;
}

bool SysInfo::getCurrentSpkMode(SpkMode* mode) {
    if (this->mtx->lock()) {
        *mode = this->spkMode;
        LOGI(LTAG, "Getting spk mode: %c", static_cast<char>(this->spkMode));
        this->mtx->unlock();
        return true;
    } else {
        *mode = SpkMode::UNKNOWN;
        return false;
    }
}

bool SysInfo::setCurrentSpkMode(SpkMode mode) {
    if (this->mtx->lock()) {
        this->spkMode = mode;
        LOGI(LTAG, "Setting spk mode: %c", static_cast<char>(this->spkMode));
        this->mtx->unlock();
        return true;
    } else {
        return false;
    }
}

bool SysInfo::getSpkId(std::array<uint8_t, 3>& id) {
    if (this->mtx->lock()) {
        id = this->spkId;
        LOGI(LTAG, "Getting SPK ID: (%02X%02X%02X)", this->spkId.at(0),
             this->spkId.at(1), this->spkId.at(2));
        this->mtx->unlock();
        return true;
    } else {
        return false;
    }
}

bool SysInfo::setSpkId(const std::array<uint8_t, 3>& id) {
    if (this->mtx->lock()) {
        this->spkId = id;
        LOGI(LTAG, "Setting SPK ID: (%02X%02X%02X)", this->spkId.at(0),
             this->spkId.at(1), this->spkId.at(2));
        this->mtx->unlock();
        return true;
    } else {
        return false;
    }
}
