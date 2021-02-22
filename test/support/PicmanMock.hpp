#pragma once

#include "comms/pic/PicmanInterface.hpp"
#include "gmock/gmock.h"

class PicmanMock : public PicmanInterface {
   public:
    MOCK_METHOD0(restart, void());
    MOCK_METHOD0(requestGroupInfo, void());
    MOCK_METHOD1(requestIdentifyAction, void(uint8_t* id));
    MOCK_METHOD1(send, void(OTAPkt& pkt));
    MOCK_METHOD1(send, void(NMEAPkt& pkt));
    MOCK_METHOD1(startBootloader, void(bool start));
    MOCK_METHOD2(sendBatteryInfo, void(uint8_t bat, BQState state));
    MOCK_METHOD1(setVolumeMaster, void(uint8_t vol));
    MOCK_METHOD0(requestVolumeMaster, void());
    MOCK_METHOD2(setVolume, void(uint8_t* id, uint8_t vol));
    MOCK_METHOD1(requestVolume, void(uint8_t* id));
    MOCK_METHOD1(sendUTADatasetSample, void(char* pdata));
    MOCK_METHOD1(sendAudioLinkStatus, void(BTLinkStatus status));
    MOCK_METHOD1(factoryReset, void(uint8_t* id));
    MOCK_METHOD1(requestAudioConfig, void(uint8_t* id));
    MOCK_METHOD2(setAudioConfig, void(uint8_t* id, uint8_t* config));
    MOCK_METHOD0(requestEqualizer, void(void));
    MOCK_METHOD1(setEqualizer, void(uint8_t eq));
    MOCK_METHOD0(requestTouchRingMode, void());
    MOCK_METHOD1(setTouchRingMode, void(uint8_t mode));
    MOCK_METHOD0(requestSpeakerMode, void(void));
    MOCK_METHOD0(disconnectFollower, void());
};
