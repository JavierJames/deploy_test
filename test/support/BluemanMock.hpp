#pragma once

#include "blue/BluemanInterface.hpp"
#include "gmock/gmock.h"

class BluemanMock : public BluemanInterface {
   public:
    MOCK_METHOD1(getFormattedMAC, bool(char* mac));
    MOCK_METHOD0(getName, const char*());
    MOCK_METHOD0(startBtPairing, void());
    MOCK_METHOD0(stopBtPairing, void());
    MOCK_METHOD0(disconnectAudioLink, void());
    MOCK_METHOD0(enableAudio, void());
    MOCK_METHOD0(disableAudio, void());
    MOCK_METHOD0(enableBeacon, void());
    MOCK_METHOD0(disableBeacon, void());
    MOCK_METHOD2(updateAdv,
                 void(const std::array<char, FW_VER_LEN>& pic,
                      const std::array<char, FW_VER_LEN>& nrf));
    MOCK_METHOD2(updateAdv, void(uint8_t battery, bool charging));
    MOCK_METHOD1(sendGroupInfo, void(GroupInfo& group));
    MOCK_METHOD0(sendGroupEvent, void());
    MOCK_METHOD1(sendMasterVolume, void(uint8_t vol));
    MOCK_METHOD1(sendTouchRingMode, void(uint8_t mode));
    MOCK_METHOD0(disableBeacon, void(0));
    MOCK_METHOD0(startBleScan, void(0));
    MOCK_METHOD1(send, void(nus_pkt_t& pkt));
    MOCK_METHOD2(sendIndivVol,
                 void(uint8_t vol, const std::array<uint8_t, 3>& id));
    MOCK_METHOD2(sendAudioConfig,
                 void(const std::array<uint8_t, 3>& conf,
                      const std::array<uint8_t, 3>& id));
    MOCK_METHOD0(sendEqualizer, void(uint8_t eq));
    MOCK_METHOD0(factoryReset, void());
};
