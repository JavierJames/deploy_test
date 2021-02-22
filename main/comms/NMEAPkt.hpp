#pragma once

#include "HAL.hpp"

#include <stdint.h>
#include "SerialPkt.hpp"

/**
 * Represents an NMEA packet used to communicate with the PIC.
 * A sample packet would be '$91,12,6a7*6B'.
 */
class NMEAPkt : public SerialPkt {
   public:
    /**
     * Enumeration of available packet pages for PIC-ESP Comms (PICCOMMS).
     */
    enum class NMEAPktPages : uint8_t {
        PICCOMMS_INVALID_PAGE = 0,
        PICCOMMS_BQ_PAGE,
        PICCOMMS_UTA_PAGE,
        PICCOMMS_CERT_PAGE,
        PICCOMMS_OTA_ESP_STATUS=5,
        PICCOMMS_RESET = 10,
        PICCOMMS_SLEEP,
        PICCOMMS_POWER_OFF,
        PICCOMMS_BT_STATUS_SET,
        _PICCOMMS_BT_DISCONNECT_LINK,  // Deprecated (use
                                       // PICCOMMS_BT_STATUS_SET)
        PICCOMMS_BT_LINK_STATUS,
        PICCOMMS_BT_LINK_STATUS_REQ = 16, // Request from PIC
        PICCOMMS_AUTO_POWEROFF_SUPPORT = 17, // Communicate if speaker should support auto power off
        PICCOMMS_STANDBY = 18,
        PICCOMMS_PSU_PLUGGED_TIMEOUT= 19,
        PICCOMMS_ACCEL_READ = 20,

        PICCOMMS_VOL_MASTER_REQ = 30,
        PICCOMMS_VOL_MASTER,
        PICCOMMS_VOL_MASTER_SET,
        PICCOMMS_VOL_INDIV_REQ,
        PICCOMMS_VOL_INDIV,
        PICCOMMS_VOL_INDIV_SET,
        PICCOMMS_FACTORY_RESET_REQ = 38,
        PICCOMMS_FACTORY_RESET = 39,
        PICCOMMS_DWAM_INFO_REQ = 40,  // Deprecated (we use DWAM pairing mode)
        PICCOMMS_DWAM_STATUS,           // Deprecated (we use DWAM pairing mode)
        PICCOMMS_DEFAULTLEADER_COCO,
        PICCOMMS_WHITELIST_COCO_ACK,
        PICCOMMS_BTMAC_ADDR_REQ,
        PICCOMMS_BTMAC_ADDR,
        PICCOMMS_BTNAME_REQ,
        PICCOMMS_BTNAME,
        PICCOMMS_SYSTEM_STATUS_REQ,
        PICCOMMS_SYSTEM_STATUS,
        PICCOMMS_SERIAL_NUM_REQ,
        PICCOMMS_SERIAL_NUM,
        PICCOMMS_KN_SESS_SPK_REQ = 60,
        PICCOMMS_KN_SESS_SPK,
        PICCOMMS_KN_SESS_SPK_ADD =
            65,  // Deprecated (use MDBTCOMMS_KN_SESS_EVT)
        PICCOMMS_KN_SESS_SPK_DEL,
        PICCOMMS_KN_SESS_SPK_FTY = 67,
        PICCOMMS_KN_SESS_SPK_EVT = 70,
        PICCOMMS_AUDIO_CONFIG_REQ = 62,
        PICCOMMS_AUDIO_CONFIG,
        PICCOMMS_AUDIO_CONFIG_SET,
        PICCOMMMS_AUDIO_EQ_REQ = 71,
        PICCOMMMS_AUDIO_EQ,
        PICCOMMMS_AUDIO_EQ_SET,
        PICCOMMMS_ACCEPT_FOLLOWER,
        PICCOMMMS_ENABLE_DWAM_CU,
        PICCOMMS_TRING_MODE_REQ = 67,
        PICCOMMS_TRING_MODE,
        PICCOMMS_TRING_MODE_SET,
        PICCOMMS_BAT_SOC_REQ = 90,
        PICCOMMS_BAT_SOC,
        PICCOMMS_SPEAKER_MODE_REQ,
        PICCOMMS_SPEAKER_MODE,
        PICCOMMS_FW_NAME_REQ,
        PICCOMMS_FW_NAME,
        PICCOMMS_FW_VERSION_REQ,
        PICCOMMS_FW_VERSION,
        PICCOMMS_OTA_PIC_STATUS,
        PICCOMMS_IDENTIFY_SPEAKER,
        PICCOMMS_UTA_DATASET_BROADCAST = 150,
        PICCOMMS_UTA_DATASET_RX_REQ,
        PICCOMMS_AUDIO_PAUSE = 80,
        PICCOMMS_AUDIO_RESUME,
    };

    NMEAPkt();

    /**
     * Builds a raw packet from the given fields.
     *
     * @param page packet type
     * @param params pointer to a \0-ended array (string) containing the packet
     * parameters. The parameters shall be separated by the delimiter (',').
     *
     * Example of usage:
     * char params[] = "12s4,0.23,foo";
     * NMEAPkt* newpkt = new NMEAPkt(NMEAPkt::NMEAPktPages::COMMS_BAT_PAGE,
     *                               params);
     */
    NMEAPkt(NMEAPkt::NMEAPktPages page, char* params);

    void encode(NMEAPkt::NMEAPktPages page, char* params);

    // Packet fields
    NMEAPktPages page;
    uint8_t chks;

    /**
     * The checksum is just an XOR of all the bytes between the '$' and the
     * '*' (not including the delimiters themselves).
     *
     * @return checksum of the packet pointed by NMEAPkt::raw
     */
    uint8_t obtainChecksum();

    /**
     * Parse the raw packet array (data, len) and initialize object members.
     *
     * @param data pointer to the raw data bytes
     * @param len number of bytes of the raw data
     * @return true if the packet is correct, false otherwise
     */
    bool parseArray(char* data, size_t len);

    /**
     * Find next parameter. This function iterates through the parameters until
     * it reaches the last one.
     *
     * @param param pointer to the first byte of the next paramter. If NULL it
     *              means there are no more parameters.
     * @return number of bytes of the parameter. If 0 it means there are no more
     *         parameters.
     */
    size_t nextParam(char** param);

    /**
     * Check if the raw packet fits in the available buffer.
     */
    bool isRawBuffAvailable();

   private:
    char* nextParamPtr;
};
