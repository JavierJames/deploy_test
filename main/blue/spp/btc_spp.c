#include "btc_spp.h"

#include "esp_gatts_api.h"


#include "time.h"
#include "sys/time.h"
#include "btc_spp_cmd.hpp"
#include "system/crc.h"

#include "esp_log.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"


static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;


static const char BT_TAG[] = "BTC_SPP";

 
static spp_pkt_t _rxbuff;

#define SPP_SERVER_NAME "SPP_SERVER"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#define SEED 0x1D0F

static uint32_t _connection_handle;
static parser_callback parser_func;

const static uint8_t escp_chars[2]={0x02,0x03};
static uint32_t crc = SEED;



static uint16_t _handle_wrapped_msg(uint8_t* src, uint16_t src_size){
    if((src==NULL) || (src_size ==0)) return 0;

    uint16_t i;
    uint8_t byte;
    static uint16_t pos = 0;
    static uint8_t prev_byte = 0;
    uint16_t num_elem = 0;

    ESP_LOGD(BT_TAG, "src_size:%d\n", src_size);

    for(i=0; i<src_size; i++){
        byte=src[i];
        if (pos >= SPP_MAX_ALLOWED) pos = 0;

        //Handle Escape characters
        if(((byte==escp_chars[0]) || (byte==escp_chars[1])) && (prev_byte == byte)) {
            prev_byte = 0;
            continue;
        }
        //confirm that the current element and next element are not the end character
        else if ((prev_byte == escp_chars[0]) && (byte == escp_chars[1])){
            prev_byte = 0;
            if(pos > 0) {
                _rxbuff.len = pos - 1;
                _rxbuff.data[pos - 1] = 0;
                parser_func(&_rxbuff);
            }
            pos = 0;
        } else {
            if (pos > 1 && _rxbuff.data[0] == 0x14) crc = byteCRCCCITT(_rxbuff.data[pos - 1], crc);
            _rxbuff.data[pos++] = byte;
            prev_byte = byte;
        }
    }
    return num_elem;
}

static uint16_t _add_escape_chars_and_end_chars(uint8_t *dest, uint16_t dest_size, uint8_t* src, uint16_t src_size){
    if((src==NULL) || (dest==NULL)  || (src_size ==0)  || (dest_size ==0) ) return 0;

    if (!(dest_size >= 2 * src_size)){
        ESP_LOGE(BT_TAG, "dest_size must be at least 2x receiver size + 3");
        return 0;
    }

    uint16_t i =0, j=0;
    uint8_t byte=0xff; //default

    for(i=0; i<src_size; i++){
        byte= src[i];
        dest[j++] = byte;


        if( (byte == escp_chars[0]) || (byte == escp_chars[1]) ) {
          dest[j++] = byte;
        }
    }

    dest[j++] = escp_chars[0];
    dest[j++] = escp_chars[1];

    return j;

}


void btc_spp_evt_handlr(esp_spp_cb_event_t event,
					esp_spp_cb_param_t *param){

    uint8_t* p_src = param->data_ind.data;
    uint16_t remainder = param->data_ind.len;


    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(BT_TAG, "ESP_SPP_INIT_EVT");
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,ESP_BT_GENERAL_DISCOVERABLE);
        esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(BT_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(BT_TAG, "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(BT_TAG, "ESP_SPP_CLOSE_EVT");
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(BT_TAG, "ESP_SPP_START_EVT");
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(BT_TAG, "ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT:
        ESP_LOGD(BT_TAG, "Raw received packet %d",remainder);

        if(SPP_WRAPPER_STATUS == SPP_WRAPPER_ON){
            _handle_wrapped_msg(p_src, remainder);
        } else {
            memcpy(_rxbuff.data, p_src, remainder);
            _rxbuff.len = remainder;
            if(_rxbuff.len >0) parser_func(&_rxbuff);
        }
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(BT_TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT:
        ESP_LOGD(BT_TAG, "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(BT_TAG, "ESP_SPP_SRV_OPEN_EVT");

		//Update the connection handle
		_connection_handle = param->write.handle;
        break;
    default:
        break;
    }
}


bool btc_spp_init(void (*parser)(spp_pkt_t* pkt))
{
    parser_func = parser;
    esp_err_t ret;

    if ((ret = esp_spp_register_callback(btc_spp_evt_handlr)) != ESP_OK) {
        ESP_LOGE(BT_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(BT_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    return true;
}

void btc_spp_init_crc() {
    crc = SEED;
    ESP_LOGI(BT_TAG, "CRC initialized 0x%04X", crc);

}

uint16_t btc_spp_crc() {
    return (uint16_t)crc;
}

bool btc_spp_deinit(){
	esp_err_t ret;

    if ((ret = esp_spp_deinit()) != ESP_OK) {
        ESP_LOGE(BT_TAG, "%s spp deinit failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

   return true;
 }


void btc_spp_send(uint8_t* buff, size_t len){
	//Don't do anything if there is no data or ESP is not configured in the right mode
  static uint8_t spp_wrapped_data [SPP_MAX_ALLOWED*2+3];
	if( (buff==NULL) || (esp_spp_mode!=ESP_SPP_MODE_CB) || (len == 0)) {
		ESP_LOGW(BT_TAG, "No SPP data to send");
	}else if(esp_spp_mode!=ESP_SPP_MODE_CB) {
		ESP_LOGW(BT_TAG, "ESP not configured in correct mode");
	}else{
		esp_err_t ret;

        if(SPP_WRAPPER_STATUS == SPP_WRAPPER_ON){

             //Reserve enough space for 1 byte CMD+ maximum escape characters + 2bytes end chars
            memset(spp_wrapped_data,0,MIN(SPP_MAX_ALLOWED*2+3, len*2 +3)); //sizeof(spp_wrapped_data));
            uint16_t new_len = _add_escape_chars_and_end_chars(spp_wrapped_data,sizeof(spp_wrapped_data),buff,len);

            if(!new_len) ESP_LOGE(BT_TAG, "Failed to decode tx packet with Escape and End chars");


            if ((ret = esp_spp_write(_connection_handle, new_len, spp_wrapped_data)) != ESP_OK) {
                ESP_LOGE(BT_TAG, "Failed to send SPP data");
            }
	   }else {

                if ((ret = esp_spp_write(_connection_handle, len, buff)) != ESP_OK) {
                     ESP_LOGE(BT_TAG, "Failed to send SPP data");
                }
        }
    }

}
