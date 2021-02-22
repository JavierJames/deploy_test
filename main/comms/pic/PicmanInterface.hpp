#pragma once

#include "NMEAPkt.hpp"
#include "OTAPkt.hpp"
#include "system/BatteryMan.hpp"

#include "blue/spp/btc_spp_cmd.hpp"

enum class BTLinkStatus : uint8_t;  // Forward declaration

/**
 * PIC manager. Initialises the serial interface and the packet parser.
 */
class PicmanInterface {
   public:
    PicmanInterface() : bootloader(false){};
    virtual ~PicmanInterface(){};

    /**
     * Request a PIC reset.
     */
    virtual void restart() = 0;

    /**
     * Request info about the speaker group.
     */
    virtual void requestGroupInfo() = 0;

    /**
     * Triggers the Identify action.
     */
    virtual void requestIdentifyAction(uint8_t* id) = 0;

    virtual void startBootloader(bool start) = 0;

    virtual void sendUTADatasetSample(char* pdata) = 0;

    /**
     * Sends a raw packet to the PIC main app.
     *
     * @param pkt packet to be sent.
     */
    virtual void send(NMEAPkt& pkt) = 0;

    /**
     * Sends a raw packet to the bootloader.
     *
     * @param pkt packet to be sent.
     */
    virtual void send(OTAPkt& pkt) = 0;

    /**
     * Sends a raw packet to the bootloader.
     *
     * @param pkt packet to be sent.
     */
    virtual void send(spp_pkt_t& pkt) = 0;

    /**
     * Sends the battery information.
     *
     * @param bat current battery voltage.
     * @param state current battery state.
     */
    virtual void sendBatteryInfo(uint8_t bat, BQState state) = 0;

    /**
     * Change a new master volume.
     *
     * @param vol percentage value (0-100)
     */
    virtual void setVolumeMaster(uint8_t vol) = 0;

    /**
     * Request the master volume information.
     */
    virtual void requestVolumeMaster() = 0;

    /**
     * Change a new master volume.
     *
     * @param vol percentage value (0-100)
     */
    virtual void setVolume(uint8_t* id, uint8_t vol) = 0;

    /**
     * Request the master volume information.
     */
    virtual void requestVolume(uint8_t* id) = 0;

    /**
     * Sends the Bluetooth link status.
     *
     * @param status current status.
     */
    virtual void sendAudioLinkStatus(BTLinkStatus status) = 0;

    /**
     * Request a factory reset to default settings.
     */
    virtual void factoryReset(uint8_t* id) = 0;

    /**
     * Request the audio configuration (equalizer, position and channel).
     */
    virtual void requestAudioConfig(uint8_t* id) = 0;

    /**
     * Modifies the audio configuration (equalizer, position and channel).
     */
    virtual void setAudioConfig(uint8_t* id, uint8_t* config) = 0;

    /**
     * Request the current equalizer settings.
     */
    virtual void requestEqualizer() = 0;

    /**
     * Set new equalizer settings.
     *
     * @param new filter.
     */
    virtual void setEqualizer(uint8_t eq) = 0;

    /**
     * Request the touch ring mode configuration.
     */
    virtual void requestTouchRingMode() = 0;

    /**
     * Modifies the touch ring mode configuration.
     */
    virtual void setTouchRingMode(uint8_t mode) = 0;

    /**
     * Request the current speaker mode.
     */
    virtual void requestSpeakerMode() = 0;

    /**
     * Request to enable the DWAM as a Central Unit
     */
    virtual void enableDWAMasCU() = 0;

    /**
     *Request to accept a new follower to pair with the L-SAT via Kleernet
     */
    virtual void acceptNewFollower() = 0;

    /**
     * Disconnect follower from leader.
     */
    virtual void disconnectFollower(uint8_t* id) = 0;

    /**
     * Send Accelerometer status
     */
    virtual void sendAccelStatus(uint8_t status) = 0;

    /**
     * Send OTA ESP status
     */
    virtual void sendOTAespStatus(FwUpdtStatus status) = 0;

    /**
     * Send sleep message
     */
    virtual void sendSleepMessage() = 0;

    /**
     * Request PIC serial number 
     */
    virtual void requestSerialNumber() = 0;

    virtual void sendStandbyMessage() =0;

    virtual void sendPSUPluggedTimeout() =0;

   protected:
    bool bootloader;
};
