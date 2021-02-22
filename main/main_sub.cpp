#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "blue/BluemanSub.hpp"
#include "blue/BTC_Blueman.hpp"
#include "comms/pic/Picman.hpp"
#include "system/BatteryMan.hpp"
#include "system/SysInfo.hpp"
#include "uta/Datasetman.hpp"
#include "uta/dataset.hpp"

#include "HAL.hpp"
#include "globals.hpp"

SysInfo& getSysInfo() {
    static Mutex mtxSysInfo;
    static SysInfo sys(&mtxSysInfo);
    return sys;
}

PicmanInterface& getPicman() {
    static Picman pic;
    return pic;
}

BluemanInterface& getBlueman() {
    static BluemanSub blue;
    return blue;
}

BTC_BluemanInterface& getBTCBlueman() {
    static BTC_Blueman btc_blue;
    return btc_blue;
}


static HAL& hal = getHAL();

static Picman& pic = static_cast<Picman&>(getPicman());
static BluemanSub& bm = static_cast<BluemanSub&>(getBlueman());
BTC_Blueman& btc_bm = static_cast<BTC_Blueman&>(getBTCBlueman());

static SysInfo& sys = getSysInfo();
static BatteryMan batman;
static Datasetman dsman;
bool isOtaBtClassic = false;


extern bool m_auto_pwr_off_enabled;

// ****************************************************************************
//                             LEADER MODE
// ****************************************************************************

static void _enable_leader_mode() {
    LOGI("LEADER", "Enable mode");
    bm.enableBeacon();
}

static void _disable_leader_mode() {
    LOGI("LEADER", "Disable mode");
    bm.disableBeacon();
}

// ****************************************************************************
//                             SLEEP MODE
// ****************************************************************************

static void _enable_sleep_mode() {
    LOGI("SLEEP", "Enable mode");
    bm.stopBleScan();
    dsman.suspend();
}

static void _disable_sleep_mode() {
    LOGI("SLEEP", "Disable mode");
    // Do not disable Picman, otherwise the mode change won't be received
    dsman.resume();
}

// *****************************************************************************
// *****************************************************************************
//                                MAIN FUNCTION
// *****************************************************************************
// *****************************************************************************

extern "C" void app_main() {
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    hal.setup();

    isOtaBtClassic = false;

    bm.init_lem();
    btc_bm.init_spp();

    pic.start();
    dsman.start();

    SpkMode new_mode = SpkMode::UNKNOWN;
    SpkMode previous_mode = SpkMode::SLEEP;

    // This request initializes the ESP after it crashes. During a normal boot
    // the PIC will send this info after initializing each mode.
    pic.requestSpeakerMode();

    while (true) {
        pic.newMode.take();
        sys.getCurrentSpkMode(&new_mode);

        if (new_mode == SpkMode::UNKNOWN) {
            continue;
        }

        if (previous_mode == SpkMode::SLEEP) {
            _disable_sleep_mode();
            if (new_mode == SpkMode::LEADER) {
                _enable_leader_mode();
            }
        } else if (previous_mode == SpkMode::LEADER) {
            _disable_leader_mode();
            if (new_mode == SpkMode::SLEEP) {
                _enable_sleep_mode();
            }
        }

        previous_mode = new_mode;
    }
}
