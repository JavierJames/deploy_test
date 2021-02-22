#include "BTC_Blueman.hpp"

#include <assert.h>

#include "globals.hpp"
static PicmanInterface& pic = getPicman();

#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"

// RAHUL
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "bt_app_av.h"
#include "bt_app_core.h"
#include "esp_spp_api.h"

#include "a2dp/bt_app_av.h"
#include "a2dp/bt_app_core.h"

#include "ble/ble_adv_conf.hpp"
#include "ble/ble_central.hpp"
#include "ble/ble_gap.hpp"
#include "ble/ble_gatt.hpp"
#include "ble/ble_peripheral.hpp"
#include "ble/profiles/ble_nus_cmd.hpp"
#include "spp/btc_spp_cmd.hpp"
#include "spp/btc_spp.h"

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

static const char BT_TAG[] = "BTC-Blueman";

static SysInfo& sys = getSysInfo();

static char bt_name[20];
 
static BTC_Blueman* instance;

BTC_Blueman::BTC_Blueman() : parser(pic_ota, esp_ota) {  

    this->create("BTC_Blueman", 4096 , 20);
    instance = this;
}

void BTC_Blueman::static_parser_call(spp_pkt_t* cmd) {
    instance->parser.parsePkt(cmd); 
}

void BTC_Blueman::init_spp() {

    //Initialize SPP
    btc_spp_init(static_parser_call);
}


void BTC_Blueman::send(spp_pkt_t& pkt) {
    btc_spp_send(pkt.data, pkt.len);
}


void BTC_Blueman::task() {
 
    for (;;) {
        this->delay(1000);
     }
    remove();
}
 