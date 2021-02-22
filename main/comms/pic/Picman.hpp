#include "PicmanInterface.hpp"

#include "PicParser.hpp"
#include "PicStream.hpp"

#include "comms/GroupInfo.hpp"
#include "comms/OTAPkt.hpp"
#include "comms/SerialPkt.hpp"
#include "system/SysInfo.hpp"

class Picman : public PicmanInterface {
   public:
    /**
     * Class constructor.
     */
    Picman();

    /**
     * Starts reading the UART and parsing the received bytes looking for
     * NMEA packets received from the PIC.
     */
    void start();

    /**
     * Semaphore to notify whenever the PIC switches to a new mode.
     */
    BinarySemaphore newMode;

    void restart() override;
    void requestGroupInfo() override;
    void requestIdentifyAction(uint8_t* id) override;
    void startBootloader(bool start) override;
    void sendBatteryInfo(uint8_t bat, BQState state) override;
    void sendUTADatasetSample(char* pdata) override;
    void send(NMEAPkt& pkt) override;
    void send(OTAPkt& pkt) override;
    void send(spp_pkt_t& pkt) override;
    void setVolumeMaster(uint8_t vol) override;
    void requestVolumeMaster() override;
    void setVolume(uint8_t* id, uint8_t vol) override;
    void requestVolume(uint8_t* id) override;
    void sendAudioLinkStatus(BTLinkStatus status) override;
    void factoryReset(uint8_t* id) override;
    void requestAudioConfig(uint8_t* id) override;
    void setAudioConfig(uint8_t* id, uint8_t* config) override;
    void requestEqualizer() override;
    void setEqualizer(uint8_t eq) override;
    void requestTouchRingMode() override;
    void enableDWAMasCU() override;
    void acceptNewFollower() override;
    void setTouchRingMode(uint8_t mode) override;
    void requestSpeakerMode() override;
    void disconnectFollower(uint8_t* id) override;
    void sendAccelStatus(uint8_t status) override;
    void sendOTAespStatus(FwUpdtStatus status) override;
    void sendSleepMessage() override;
    void requestSerialNumber() override;
    void sendStandbyMessage() override;

    void sendPSUPluggedTimeout() override;

   private:
    PicParser parser;
    PicStream stream;
};
