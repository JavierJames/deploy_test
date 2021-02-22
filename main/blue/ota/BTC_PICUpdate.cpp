#include "BTC_PICUpdate.hpp"

#include <assert.h>

#include "globals.hpp"
static PicmanInterface& pic = getPicman();
static BTC_BluemanInterface& btc_blue = getBTCBlueman();


#include "HAL.hpp"
#include "comms/NMEAPkt.hpp"
#include "comms/OTAPkt.hpp"

#define NUM_ATTEMPTS_BOOTLOADER_TRIGGER 10

static HAL& hal = getHAL();

static const char LTAG[] = "BTC-PIC-OTA";

bool BTC_PICUpdate::isBootloaderReady() {
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

void BTC_PICUpdate::start(const btc_ota_init_pkt_t& init) {
    BTC_OTAUpdate::start(init);

    hal.gpio->set(ESP_PIC_PIN);
    NMEAPkt pkt(NMEAPkt::NMEAPktPages::PICCOMMS_OTA_PIC_STATUS, NULL);
    pic.send(pkt);

    if (isBootloaderReady()) {
        // Tell app update can begin
        Task::delay(200);
        p_tx_pkt->data[0] = BTC_SPP_CMD_PIC_OTA_BEGIN_RES;
        p_tx_pkt->data[1] = 0;
        p_tx_pkt->len = 2;
        btc_blue.send(*p_tx_pkt);
        pic.startBootloader(true);
        LOGI(LTAG, "PIC ready for OTA");
    } else {
        // Tell app update has failed to start
        p_tx_pkt->data[0] = BTC_SPP_CMD_PIC_OTA_BEGIN_RES;
        p_tx_pkt->data[1] = 1;
        p_tx_pkt->len = 2;
        btc_blue.send(*p_tx_pkt);
        pic.startBootloader(false);
        LOGW(LTAG, "PIC not ready for OTA");
    }

    Task::delay(200);
    // We can clear the line because the bootloader has already started
    hal.gpio->clear(ESP_PIC_PIN);
}

void BTC_PICUpdate::processPkt(spp_pkt_t& pkt) {
    static spp_pkt_t ota;

    //Todo handle sending packets larger 
    assert((pkt.len - 1) < SPP_MAX_ALLOWED);

    memcpy(ota.data,pkt.data + 1,pkt.len - 1);
    ota.len = pkt.len -1; 
    pic.send(ota);
    BTC_OTAUpdate::processPkt(pkt);
}

void BTC_PICUpdate::abort() {
    pic.startBootloader(false);
    BTC_OTAUpdate::abort();
}

void BTC_PICUpdate::finish(bool success) {
    pic.startBootloader(false);
    BTC_OTAUpdate::finish(success);
}
