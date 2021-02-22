#pragma once

#include "HAL.hpp"

#include "ble/ble_adv_conf.hpp"
#include "comms/GroupInfo.hpp"
#include "ota/BTC_ESPUpdate.hpp"
#include "ota/BTC_PICUpdate.hpp"
#include "system/SysInfo.hpp"


class BTC_BluemanInterface {
   protected:
    Mutex adv_mtx;
    Kien::altbeacon_t beacon;
    Kien::scan_response_t scanres;

   public:
    BTC_PICUpdate pic_ota;
    BTC_ESPUpdate esp_ota;
    virtual ~BTC_BluemanInterface(){};

    /**
     * Updates the firmware versions in the BLE advertisement.
     *
     * @param pic PIC32's firmware version.
     * @param esp ESP32's firmware version.
     *
    virtual void updateAdv(const std::array<char, FW_VER_LEN>& pic,
                           const std::array<char, FW_VER_LEN>& nrf) = 0;
    */
    
    /**
     * Send a SPP packet.
     *
     * @param pkt SPP packet to be sent.
     */
    virtual void send(spp_pkt_t& pkt) = 0;
};
