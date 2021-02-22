#pragma once

#include "HAL.hpp"

#include "ble/ble_adv_conf.hpp"
#include "comms/GroupInfo.hpp"
#include "ota/ESPUpdate.hpp"
#include "ota/PICUpdate.hpp"
#include "system/SysInfo.hpp"

enum class BTLinkStatus : uint8_t {
    DISCONNECTED = 0,
    DISCONNECTING,
    CONNECTED,
    CONNECTING
};

class BluemanInterface {
   protected:
    Mutex adv_mtx;
    Kien::altbeacon_t beacon;
    Kien::scan_response_t scanres;

   public:
    PICUpdate pic_ota;
    ESPUpdate esp_ota;
    virtual ~BluemanInterface(){};

    /**
     * Request Bluetooth MAC address and writes it to the given pointer.
     * The array has to be allocated by the caller with 6 bytes.
     *
     * @returns true if the address was copied, false otherwise.
     */
    virtual bool getFormattedMAC(char* mac) = 0;

    /**
     * Request Bluetooth name.
     *
     * @returns '\0'-ended pointer to the name; Null if something went wrong.
     */
    virtual const char* getName() = 0;

    /**
     * Start BT Classic pairing mode (device discoverable and connectable).
     * Note: This function must be called after Blueman::initdm();
     */
    virtual void startBtPairing() = 0;

    /**
     * Stop BT Classic pairing mode (device connectable but not discoverable).
     * Note: This function must be called after Blueman::initdm();
     */
    virtual void stopBtPairing() = 0;

    /**
     * Disconnects the active BT link (A2DP source).
     */
    virtual void disconnectAudioLink() = 0;

    /**
     * Enables BT Classic (A2DP and AVRCP profiles) and sets it connectable but
     * not discoverable.
     */
    virtual void enableAudio() = 0;

    /**
     * Disables BT Classic (neither connectable nor discoverable).
     */
    virtual void disableAudio() = 0;

    /**
     * Enables BLE beacon and services.
     */
    virtual void enableBeacon() = 0;

    /**
     * Disables BLE beacon and services.
     */
    virtual void disableBeacon() = 0;

    /**
     * Starts looking for (Kien) BLE beacons.
     */
    virtual void startBleScan() = 0;

    /**
     * Stops looking for (Kien) BLE beacons.
     */
    virtual void stopBleScan() = 0;

    /**
     * Updates the firmware versions in the BLE advertisement.
     *
     * @param pic PIC32's firmware version.
     * @param esp ESP32's firmware version.
     */
    virtual void updateAdv(const std::array<char, FW_VER_LEN>& pic,
                           const std::array<char, FW_VER_LEN>& nrf) = 0;

    /**
     * Updates the battery status in the BLE advertisement.
     *
     * @param battery battery level (0-100).
     * @param charging true when charging, false otherwise.
     */
    virtual void updateAdv(uint8_t battery, bool charging) = 0;

    /**
     * Sends group information commands through BLE.
     *
     * @param group object containing group information.
     */
    virtual void sendGroupInfo(GroupInfo& group) = 0;

    /**
     * Sends group event command through BLE.
     */
    virtual void sendGroupEvent() = 0;

    /**
     * Sends speaker information through BLE.
     *
     * @param spk object containing speaker information.
     */
    virtual void sendSpkInfo(GroupMemberInfo& spk) = 0;

    /**
     * Sends the master volume command through BLE.
     *
     * @param vol master volume.
     */
    virtual void sendMasterVolume(uint8_t vol) = 0;

    /**
     * Sends the individual volume command through BLE.
     *
     * @param vol paster volume.
     * @param id speaker id to which the volume corresponds.
     */
    virtual void sendIndivVol(uint8_t vol,
                              const std::array<uint8_t, 3>& id) = 0;

    /**
     * Sends the audio settings through BLE.
     *
     * @param conf array containing equalizer, position and channel settings.
     * @param id speaker id to which the volume corresponds.
     */
    virtual void sendAudioConfig(const std::array<uint8_t, 3>& conf,
                                 const std::array<uint8_t, 3>& id) = 0;

    /**
     * Sends the audio equalizer configuration through BLE.
     *
     * @param eq current equalizer config.
     */
    virtual void sendEqualizer(uint8_t eq) = 0;

    /**
     * Clears the whitelisted bluetooth devices.
     */
    virtual void factoryReset() = 0;

    /**
     * Sends the DWAM status
     *
     * @param status 0 for not ready, 1 for ready
     */
    virtual void sendDWAMStatus(uint8_t status) = 0;

    /**
     * Sends the touch ring control mode.
     *
     * @param mode 0 for master volume, 1 for individual volume, 2 for space
     * volume.
     */
    virtual void sendTouchRingMode(uint8_t mode) = 0;

    /**
     * Sends the serial number.
     *
     * @param serial_number 10 byte serial number to be sent.
     */
    virtual void sendSerialNumber(uint8_t* serial_number) = 0;

    /**
     * Send a packet through the NUS service.
     *
     * @param pkt NUS packet to be sent.
     */
    virtual void send(nus_pkt_t& pkt) = 0;
  
};
