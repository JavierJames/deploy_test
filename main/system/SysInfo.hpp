#pragma once

#include "HAL.hpp"

#include <array>

#define ESP_FW_NAME "v0.0.7" 
#define ESP_FW_VERSION \
    { '0', '0', '7' } 
#define PIC_FW_VERSION_ERROR \
    { 'f', 'f', 'f' }
#define SPK_ID_DEFAULT \
    { 0, 0, 0 }
#define PIC_FW_NAME_MAX_LEN 20
#define FW_VER_LEN 3

#define SERIAL_NUM_LEN 10

#define STANDBY_PSU_PLUGGED_TIMEOUT_MINUTES 15 //Amount of minutes that the speaker should power off when in standby mode if the PSU is not plugged
#define STANDBY_PSU_PLUGGED_TIMEOUT_MS     STANDBY_PSU_PLUGGED_TIMEOUT_MINUTES * 60 * 1000  


// bool m_auto_pwr_off_enabled;
/**
 * Speaker modes.
 */
enum class SpkMode : char {
    UNKNOWN = 'E',
    BOOT = 'B',
    STANDBY = 'G',
    LEADER = 'L',
    FOLLOWER = 'F',
    SLEEP = 'S'
};

/**
 * Thread-safe class to access system info by the different tasks (methods use a
 * Mutex to access the resources).
 */
class SysInfo {
   public:
    /**
     * Class constructor.
     *
     * @param _mtx pointer to a mutex which will be used by the class to access
     * the internal variables. It's an injected dependency to allow unit tests
     * to be run.
     */
    explicit SysInfo(Mutex* _mtx);

    /**
     * Get serial number, if NULL, serial number is requested from PIC
     *
     * @return serial number.
     */
    uint8_t* getSerialNumber();

    /**
     * Set PIC serial number
     *
     * @param serial_number.
     * @return true if serial number successfully set; otherwise, false.
     */
    bool setSerialNumber(uint8_t* serial_number);

    /**
     * Is serial number loaded from PIC
     *
     * @return true if loaded.
     */
    bool isSerialNumberLoaded();

    /**
     * Get PIC firmware name. Name is empty by default.
     *
     * @param name pointer to a buffer. The function will copy (memcpy) the
     *             value, so the buffer must be initialized by the caller.
     *
     * @return lenght (bytes) of the name.
     */
    size_t getPicFwName(char* name);

    /**
     * Get PIC firmware version. Version is empty by default.
     *
     * @param ver array to return the version.
     *
     * @return lenght (bytes) of the version.
     */
    bool getPicFwVersion(std::array<char, FW_VER_LEN>& ver);

    /**
     * Set PIC firmware name.
     *
     * @param name pointer to an array containing the name.
     * @param len number of bytes to copy (length of the name).
     *
     * @return true if name set correctly, false otherwise.
     */
    bool setPicFwName(const char* name, size_t len);

    /**
     * Set PIC firmware version.
     *
     * @param ver array containing the version.
     *
     * @return true if version set correctly, false otherwise.
     */
    bool setPicFwVersion(const std::array<char, FW_VER_LEN>& ver);

    /**
     * Get ESP firmware name. Name is empty by default.
     *
     * @param name pointer to a buffer. The function will copy (memcpy) the
     *             value, so the buffer must be initialized by the caller.
     *
     * @return lenght (bytes) of the name.
     */
    size_t getEspFwName(char* name);

    /**
     * Get PIC firmware version. Version is empty by default.
     *
     * @param ver array to return the version.
     *
     * @return lenght (bytes) of the version.
     */
    void getEspFwVersion(std::array<char, FW_VER_LEN>& ver);

    /**
     * Get current speaker mode. Mode is SpkMode::UNKNOWN by default.
     *
     * @param mode pointer to a buffer. The function will copy (memcpy) the
     *             value, so the buffer must be initialized by the caller.
     *
     * @return true if version set correctly, false otherwise.
     */
    bool getCurrentSpkMode(SpkMode* mode);

    /**
     * Set speaker mode.
     *
     * @param mode Current mode.
     *
     * @return true if version set correctly, false otherwise.
     */
    bool setCurrentSpkMode(SpkMode mode);

    /**
     * Get speaker ID.
     *
     * @param ver array to return the version.
     *
     * @return lenght (bytes) of the version.
     */
    bool getSpkId(std::array<uint8_t, 3>& id);

    /**
     * Set speaker ID.
     *
     * @param ver array containing the version.
     *
     * @return true if version set correctly, false otherwise.
     */
    bool setSpkId(const std::array<uint8_t, 3>& id);

   private:
    // Variables corresponding to the PIC firmware name
    char picFwName[PIC_FW_NAME_MAX_LEN];
    size_t picFwNameLen;

    // Variables corresponding to the PIC firmware version
    std::array<char, FW_VER_LEN> picFwVer;

    // Variables corresponding to the ESP firmware name
    const char espFwName[sizeof(ESP_FW_NAME)];
    const size_t espFwNameLen;

    // Variables corresponding to the ESP firmware version
    const std::array<char, FW_VER_LEN> espFwVer;

    // Speaker mode
    SpkMode spkMode;

    // Variables corresponding to the Speaker ID
    std::array<uint8_t, 3> spkId;

    /**
     * Mutex used internally to access the variables
     */
    Mutex* mtx;
};
