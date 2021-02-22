#pragma once

#include "BluemanInterface.hpp"

#include "BleParser.hpp"
#include "BleStream.hpp"
#include "esp_system.h"
#include "bt_app_av.h"
#include "bt_app_core.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"

class BluemanSat : public BluemanInterface, public Task {
   public:
    /**
     * Class constructor.
     */
    BluemanSat();

    /**
     * Init Bluetooth Dual Mode (LE and A2DP)
     */
    void init_dm();

    bool getFormattedMAC(char* mac) override;
    const char* getName() override;
    void startBtPairing() override;
    void stopBtPairing() override;
    void disconnectAudioLink() override;
    void enableAudio() override;
    void disableAudio() override;
    void enableBeacon() override;
    void disableBeacon() override;
    void startBleScan() override;
    void stopBleScan() override;
    void updateAdv(const std::array<char, FW_VER_LEN>& pic,
                   const std::array<char, FW_VER_LEN>& nrf) override;
    void updateAdv(uint8_t battery, bool charging) override;
    void sendGroupInfo(GroupInfo& group) override;
    void sendGroupEvent() override;
    void sendSpkInfo(GroupMemberInfo& spk) override;
    void sendMasterVolume(uint8_t vol) override;
    void sendIndivVol(uint8_t vol, const std::array<uint8_t, 3>& id) override;
    void sendAudioConfig(const std::array<uint8_t, 3>& conf,
                         const std::array<uint8_t, 3>& id) override;
    void sendEqualizer(uint8_t eq) override;
    void sendTouchRingMode(uint8_t mode) override;
    void factoryReset() override;
    void sendDWAMStatus(uint8_t status) override;
    void sendSerialNumber(uint8_t* serial_number) override;

    void send(nus_pkt_t& pkt) override;

   private:
    BleParser parser;
    BleStream stream;
    BinarySemaphore audioLinkEvt;

    void task() override;
};
