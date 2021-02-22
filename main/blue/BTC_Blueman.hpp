#pragma once

#include "BTC_BluemanInterface.hpp"

#include "BTC_Parser.hpp"
#include "esp_system.h"
#include "bt_app_av.h"
#include "bt_app_core.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"



class BTC_Blueman : public BTC_BluemanInterface, public Task {
   public:
    /**
     * Class constructor.
     */
    BTC_Blueman();


    /**
     * Init Bluetooth SPP
     */
    void init_spp();
    static void static_parser_call(spp_pkt_t* cmd);
    void send(spp_pkt_t& pkt) override;

   private:
    BTC_Parser parser;

    void task() override;

};
