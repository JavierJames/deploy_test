#include "BTC_Parser.hpp"

#include <assert.h>
#include "globals.hpp"
static PicmanInterface& pic = getPicman();

#include "ble/profiles/ble_nus_cmd.hpp"
#include "spp/btc_spp_cmd.hpp"

#include "esp_log.h"

static const char BT_TAG[] = "BTC-Parser";

#include "system/SysInfo.hpp"
static SysInfo& sys = getSysInfo();

static void _handle_pkt_err(uint8_t id) {
    LOGE(BT_TAG, "Wrong pkt 0x%02X", id);
}

static void _log_id(uint8_t* id) {
    LOGI(BT_TAG, " - SPK: 0x%02X%02X%02X", id[2], id[1], id[0]);
}

uint8_t _end_[2] = {0x02,0x03};



static bool _find_end_chars_remove2(bool *found, uint16_t* new_size, uint8_t* buff, uint16_t size){
    if( (buff==NULL) || (size==0)) return false;


    if((buff[size-2] == _end_[0]) && (buff[size-1] == _end_[1])){
        *found = true;

        buff[size-1] = 0x00;
        buff[size-2] = 0x00;
        *new_size = size - 2;

        return true;

    }
    else {
        *found = false;
        return true;
    }
}


static bool _remove_end_chars(spp_pkt_t& dest, spp_pkt_t& src){
    if(src.len==NULL)  return false;

         //Check if the last 2 bytes are the end characters
    if((src.data[src.len-2] == _end_[0]) && (src.data[src.len-1] == _end_[1])){

        //Copy all the elemetns except the END characters
        memcpy(dest.data,src.data,src.len-2);
        dest.len = src.len-2;
    }
    else{

        //Copy all the data
        memcpy(dest.data,src.data,src.len);
        dest.len = src.len;
    }
    return true;
}


BTC_Parser::BTC_Parser(BTC_PICUpdate& _pic_ota, BTC_ESPUpdate& _esp_ota)
    : pic_ota(_pic_ota), esp_ota(_esp_ota), active_ota(nullptr) {}

void BTC_Parser::parsePkt(spp_pkt_t* pkt) {       
    assert(pkt);

    static bool primary_pkt = true; //True if current pkt is the first one with the command. False if message is split amongst several packets
    static uint8_t cmd= 0xff; //Set as a random packet
    bool found=true;
    uint16_t temp_size =0;


    //If this is the first packet, which containts the commands, parse command.
    //Otherwise, used previous command to process more data from the Queue  using the same command
    if(primary_pkt){
      cmd = pkt->data[0]; 

    }
 
    //If packet is a command, process it
    switch (cmd) {

        case BTC_SPP_CMD_PIC_OTA_BEGIN:
            if (pkt->len == sizeof(btc_ota_init_pkt_t) + 1) {
                pic_ota.start(
                    *reinterpret_cast<btc_ota_init_pkt_t*>(&(pkt->data[1])));
                active_ota = &pic_ota;
            } else {
                LOGE(BT_TAG, "Invalid OTA init pkt (%i)", pkt->len);
            }
            break;

        case BTC_SPP_CMD_ESP_OTA_BEGIN:
            if (pkt->len == sizeof(btc_ota_init_pkt_t) + 1) {
                esp_ota.start(
                    *reinterpret_cast<btc_ota_init_pkt_t*>(&(pkt->data[1])));
                active_ota = &esp_ota;
            } else {
                LOGE(BT_TAG, "Invalid OTA init pkt (%i)", pkt->len);
            }
            break;

        case BTC_SPP_CMD_OTA_CHUNK:
            if (active_ota != nullptr) {
                ESP_LOGD(BT_TAG, "Processing chunk");

                active_ota->processPkt(*pkt); //_parse_pkt);
            } else {
                LOGE(BT_TAG, "Update has not started");
            }
            break;

        case BTC_SPP_CMD_OTA_ABORT:
            if (active_ota != nullptr) {
                active_ota->abort();
                active_ota = nullptr;
            } else {
                LOGW(BT_TAG, "Update has not started");
            }
            break;


        default:
            LOGW(BT_TAG, "Unknown CMD 0x%02X", pkt->data[0]);
            break;
    }

}
