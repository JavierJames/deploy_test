#include "BTC_ESPUpdate.hpp"

#include "esp_ota_ops.h"

#include "HAL.hpp"

#include "globals.hpp"
static PicmanInterface& pic = getPicman();
static BTC_BluemanInterface& btc_blue = getBTCBlueman();


static const char LTAG[] = "BTC-ESP-OTA";

esp_ota_handle_t btc_update_handle = 0;
const esp_partition_t* btc_update_partition = NULL;

void BTC_ESPUpdate::start(const btc_ota_init_pkt_t& init) {
    BTC_OTAUpdate::start(init);
    // Find OTA update partition
    btc_update_partition = esp_ota_get_next_update_partition(NULL);
    if (btc_update_partition == NULL) {
        LOGE(LTAG, "Parition for update not found");
        p_tx_pkt->data[0] = BTC_SPP_CMD_ESP_OTA_BEGIN_RES;
        p_tx_pkt->data[1] = 1;
        p_tx_pkt->len = 2;
        btc_blue.send(*p_tx_pkt);
        Task::delay(200);
        return;
    }
    LOGI(LTAG, "Writing to partition subtype %d at offset 0x%x",
         btc_update_partition->subtype, btc_update_partition->address);

    // Initilizing memory
    if (esp_ota_begin(btc_update_partition, OTA_SIZE_UNKNOWN, &btc_update_handle) !=
        ESP_OK) {
        LOGE(LTAG, "OTA cannot begin");
        p_tx_pkt->data[0] = BTC_SPP_CMD_ESP_OTA_BEGIN_RES;
        p_tx_pkt->data[1] = 2;
        p_tx_pkt->len = 2;
        btc_blue.send(*p_tx_pkt);
        Task::delay(200);
        return;
    }

    p_tx_pkt->data[0] = BTC_SPP_CMD_ESP_OTA_BEGIN_RES;
    p_tx_pkt->data[1] = 0;
    p_tx_pkt->len = 2;
    btc_blue.send(*p_tx_pkt);
    Task::delay(200);

    LOGI(LTAG, "Starting OTA");
    pic.sendOTAespStatus(FwUpdtStatus::START);

}

void BTC_ESPUpdate::processPkt(spp_pkt_t& pkt) {
    BTC_OTAUpdate::processPkt(pkt);

    if (esp_ota_write(btc_update_handle, static_cast<const void*>(pkt.data + 1),
                      pkt.len - 1) != ESP_OK) {
        LOGE(LTAG, "Write has failed");
        this->finish(false);
        return;
    } else if (this->recv_len == this->total_len) {
        this->finish(true);
    }
}

void BTC_ESPUpdate::finish(bool success) {
    if (success) {
        if (esp_ota_end(btc_update_handle) != ESP_OK) {
            LOGE(LTAG, "OTA failed");
            BTC_OTAUpdate::finish(false);
            pic.sendOTAespStatus(FwUpdtStatus::FAIL);
            return;
        }

        if (esp_ota_set_boot_partition(btc_update_partition) != ESP_OK) {
            LOGE(LTAG, "Failed to update partition");
            BTC_OTAUpdate::finish(false);
            return;
        }
    }
    pic.sendOTAespStatus(FwUpdtStatus::COMPLETE);
    BTC_OTAUpdate::finish(success);
    LOGW(LTAG, "Prepare system to restart!");
    pic.restart();  // Request PIC reset (which will reset the ESP)
}

void BTC_ESPUpdate::abort() {
    if (esp_ota_end(btc_update_handle) != ESP_OK) {
        LOGE(LTAG, "OTA failed");
        pic.sendOTAespStatus(FwUpdtStatus::FAIL);
        Task::delay(1000);
        pic.restart();
    }

    BTC_OTAUpdate::abort();
}
