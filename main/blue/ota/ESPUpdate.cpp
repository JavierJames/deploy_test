#include "ESPUpdate.hpp"

#include "esp_ota_ops.h"

#include "HAL.hpp"

#include "globals.hpp"
static PicmanInterface& pic = getPicman();
static BluemanInterface& ble = getBlueman();

static const char LTAG[] = "ESP-OTA";

esp_ota_handle_t update_handle = 0;
const esp_partition_t* update_partition = NULL;

void ESPUpdate::start(const ota_init_pkt_t& init) {
    OTAUpdate::start(init);
    // Find OTA update partition
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        LOGE(LTAG, "Parition for update not found");
        nuspkt.data[0] = BLE_NUS_CMD_ESP_OTA_BEGIN_RES;
        nuspkt.data[1] = 1;
        nuspkt.len = 2;
        ble.send(nuspkt);
        Task::delay(200);
        return;
    }
    LOGI(LTAG, "Writing to partition subtype %d at offset 0x%x",
         update_partition->subtype, update_partition->address);

    // Initilizing memory
    if (esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle) !=
        ESP_OK) {
        LOGE(LTAG, "OTA cannot begin");
        nuspkt.data[0] = BLE_NUS_CMD_ESP_OTA_BEGIN_RES;
        nuspkt.data[1] = 2;
        nuspkt.len = 2;
        ble.send(nuspkt);
        Task::delay(200);
        return;
    }

    nuspkt.data[0] = BLE_NUS_CMD_ESP_OTA_BEGIN_RES;
    nuspkt.data[1] = 0;
    nuspkt.len = 2;
    ble.send(nuspkt);
    Task::delay(200);

    LOGI(LTAG, "Starting OTA");
    pic.sendOTAespStatus(FwUpdtStatus::START);

}

void ESPUpdate::processPkt(nus_pkt_t& pkt) {
    OTAUpdate::processPkt(pkt);

    if (esp_ota_write(update_handle, static_cast<const void*>(pkt.data + 1),
                      pkt.len - 1) != ESP_OK) {
        LOGE(LTAG, "Write has failed");
        this->finish(false);
        return;
    } else if (this->recv_len == this->total_len) {
        this->finish(true);
    }
}

void ESPUpdate::finish(bool success) {
    if (success) {
        if (esp_ota_end(update_handle) != ESP_OK) {
            LOGE(LTAG, "OTA failed");
            OTAUpdate::finish(false);
            pic.sendOTAespStatus(FwUpdtStatus::FAIL);
            return;
        }

        if (esp_ota_set_boot_partition(update_partition) != ESP_OK) {
            LOGE(LTAG, "Failed to update partition");
            OTAUpdate::finish(false);
            return;
        }
    }
    pic.sendOTAespStatus(FwUpdtStatus::COMPLETE);
    OTAUpdate::finish(success);
    LOGW(LTAG, "Prepare system to restart!");
    pic.restart();  // Request PIC reset (which will reset the ESP)
}

void ESPUpdate::abort() {
    if (esp_ota_end(update_handle) != ESP_OK) {
        LOGE(LTAG, "OTA failed");
        pic.sendOTAespStatus(FwUpdtStatus::FAIL);
        Task::delay(1000);
        pic.restart();
    }

    OTAUpdate::abort();
}
