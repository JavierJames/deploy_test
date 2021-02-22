#include "PICUpdate.hpp"

#include <assert.h>

#include "globals.hpp"
static PicmanInterface& pic = getPicman();
static BluemanInterface& ble = getBlueman();

#include "HAL.hpp"
#include "comms/NMEAPkt.hpp"
#include "comms/OTAPkt.hpp"

#define NUM_ATTEMPTS_BOOTLOADER_TRIGGER 10

static HAL& hal = getHAL();

static const char LTAG[] = "PIC-OTA";

bool PICUpdate::isBootloaderReady() {
    uint8_t attempts = NUM_ATTEMPTS_BOOTLOADER_TRIGGER;
    LOGI(LTAG, "Waiting for PIC bootloader");
    while (hal.gpio->get(PIC_ESP_PIN) == ePinLow) {
        if (attempts--) {
            LOGD(LTAG, ".");
            Task::delay(200);
        } else {
            return false;
        }
    }

    return true;
}

void PICUpdate::start(const ota_init_pkt_t& init) {
    OTAUpdate::start(init);

    hal.gpio->set(ESP_PIC_PIN);
    NMEAPkt pkt(NMEAPkt::NMEAPktPages::PICCOMMS_OTA_PIC_STATUS, NULL);
    pic.send(pkt);

    if (isBootloaderReady()) {
        // Tell app update can begin
        Task::delay(200);
        nuspkt.data[0] = BLE_NUS_CMD_PIC_OTA_BEGIN_RES;
        nuspkt.data[1] = 0;
        nuspkt.len = 2;
        ble.send(nuspkt);
        pic.startBootloader(true);
        LOGI(LTAG, "PIC ready for OTA");
    } else {
        // Tell app update has failed to start
        nuspkt.data[0] = BLE_NUS_CMD_PIC_OTA_BEGIN_RES;
        nuspkt.data[1] = 1;
        nuspkt.len = 2;
        ble.send(nuspkt);
        pic.startBootloader(false);
        LOGW(LTAG, "PIC not ready for OTA");
    }

    Task::delay(200);
    // We can clear the line because the bootloader has already started
    hal.gpio->clear(ESP_PIC_PIN);
}

void PICUpdate::processPkt(nus_pkt_t& pkt) {
    static OTAPkt ota;

    assert((pkt.len - 1) < SERIAL_RAW_MAX_LEN);

    ota.encode(pkt.data + 1, pkt.len - 1);
    pic.send(ota);
    OTAUpdate::processPkt(pkt);
}

void PICUpdate::abort() {
    pic.startBootloader(false);
    OTAUpdate::abort();
}

void PICUpdate::finish(bool success) {
    pic.startBootloader(false);
    OTAUpdate::finish(success);
}
