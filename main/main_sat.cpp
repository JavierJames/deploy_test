#include "esp_system.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "blue/BluemanSat.hpp"
#include "blue/BTC_Blueman.hpp"
#include "comms/pic/Picman.hpp"
#include "system/BatteryMan.hpp"
#include "system/SysInfo.hpp"
#include "uta/Datasetman.hpp"
#include "uta/dataset.hpp"
#include "peripherals/LIS2DE12.hpp"
#include "system/standby_timer.h"

#include "HAL.hpp"
#include "globals.hpp"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bt_app_av.h"
#include "a2dp/bt_app_av.h"
#include "a2dp/bt_app_core.h"
#include "a2dp/bt_whitelist.h"
#include <unistd.h>
 
 
extern bool m_auto_pwr_off_enabled;
 
typedef enum{
        STANDBY_NKNOWN,
        STANDBY_LEADER,
        STANDBY_FOLLOWER,
        NO_STANDBY,
}standby_mode_enum_t;
standby_mode_enum_t standby_mode=STANDBY_NKNOWN;

uint16_t event;
void* p_param;

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
    static BluemanSat blue;
    return blue;
}

BTC_BluemanInterface& getBTCBlueman() {
    static BTC_Blueman btc_blue;
    return btc_blue;
}

static HAL& hal = getHAL();

static Picman& pic = static_cast<Picman&>(getPicman());
BluemanSat& bm = static_cast<BluemanSat&>(getBlueman());
BTC_Blueman& btc_bm = static_cast<BTC_Blueman&>(getBTCBlueman());
static SysInfo& sys = getSysInfo();
static BatteryMan batman;
static Datasetman dsman;

static bool acc_status;

static bool sleepmode = false;
static bool followermode = false;
static bool standbypmode = false;

bool i2s_is_paused = false;
bool isOtaBtClassic = false;

SpkMode current_mode;
SpkMode new_mode;


bool standby_psu_unplugged_timeout = false;
bool wakeup_from_ext_source = false;


bool enable_sleep = false;

bool m_last_status_psu_plugged = false;



static uint64_t total_elapsed_time_sec = 0;
static uint64_t total_elapsed_bias_ms = 0;



// ****************************************************************************
//                             Misc. functions
// ****************************************************************************

static void _increment_elapsed_time_with_bias(uint32_t bias_time_ms){

    //Get timer group 0 elapsed time
    uint64_t time_sec = (uint64_t) (standby_timer_elapsed_time_in_minutes()*60);

    //Add bias to total elapsed time
    total_elapsed_bias_ms += bias_time_ms;

    //Calculate total elapsed time
    total_elapsed_time_sec = time_sec + total_elapsed_bias_ms/1000;
}

static double _standby_get_total_elapsed_time_min(){

    return ((double) total_elapsed_time_sec ) /60;

}

static void _clear_elapsed_times(void){
    total_elapsed_time_sec = 0;
    total_elapsed_bias_ms = 0;   
}

 
// ****************************************************************************
//                             LEADER MODE
// ****************************************************************************

static void _enable_leader_mode() {
    LOGI("LEADER", "Enable mode");

    bt_a2d_set_leader_mode_enabled(true);
    bt_a2d_set_signal_lost_mode(false);

    hal.i2sOut->begin(44100, 32);

    printf("In enable_leader_mode\n");

    if (followermode) {
        bm.disconnectAudioLink();

        /* wait extra PIC */
        sleep(2);
       
        //bm.delay(5000);                // Arash found that this should be here and 5000 ms

        printf("In enable_leader_mode after follower mode\n");
        followermode = false;
    }

    /* wait for PIC */
    //TODO: add message that PIC is ready to connect
    // sleep(4);

    bm.enableAudio();
    // bm.enableBeacon();                   // TO DO Uncomment if including
}

static void _disable_leader_mode() {
    LOGI("LEADER", "Disable mode");
    bt_a2d_set_leader_mode_enabled(false);

    printf("In disable_leader_mode\n");
    i2s_is_paused = true;
    sleep(.5);

    hal.i2sOut->close();
    bm.disconnectAudioLink();

    /* wait to collect handle before disabling the btc */
    sleep(1);

    bm.disableAudio();               // If taken out then the BT connection resumes automatically in Leader mode and Heavy Metal Noise occurs (though might be because disconnect hasn't occured)
    // bm.disableBeacon();             // TO DO Uncomment if including
}

// ****************************************************************************
//                             FOLLOWER MODE
// ****************************************************************************

static void _enable_follower_mode() {
    LOGI("FOLLOWER", "Enable mode");
    followermode = true;

    printf("In enable_follower_mode\n");

    bm.suspend();               // This is in place to prevent the blue led from turning on during follower mode
}

static void _disable_follower_mode() {
    LOGI("FOLLOWER", "Disable mode");

    printf("In disable_follower_mode\n");

    bm.resume();                   // This is to allow automatic reconnection after the mode change

    // bm.disableBeacon();           // TO DO Uncomment if including
    // bm.disconnectAudioLink();     // TO DO Uncomment if including

}

// ****************************************************************************
//                             SLEEP MODE
// ****************************************************************************

static void _enable_sleep_mode() {
    LOGI("SLEEP", "Enable mode");

    /* This sleep is required to make sure every current 
     * event is handled and NVS is done writing before sleeping 
     * if removed causes crashes when connected with both BLE and BTC*/
    //TODO: try to reduce
    sleep(2);

    sleepmode = true;

    batman.suspend();
    dsman.suspend();
    bm.suspend();

    // bm.stopBleScan();             // TO DO Uncomment if including

    /* This sleep is required to make sure every task is suspended */
    //TODO: try to reduce
    sleep(4);

    pic.sendSleepMessage();

    /* Give the package some time to send */
    sleep(2);

    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    printf("exit sleep mode, it is time to work!\n");
}

static void _disable_sleep_mode() {
    LOGI("SLEEP", "Disable mode");
    // Do not disable Picman, otherwise the mode change won't be received

    // bm.enable();

    bt_whitelist_nvs_load();

    if (sleepmode) {
        //bm.init_dm();

        hal.setup();
        
        // This request initializes the ESP after it crashes. During a normal boot
        // the PIC will send this info after initializing each mode.
        pic.requestSpeakerMode();

        printf("Recovery from sleepmode\n");

        sleepmode = false;
    }

    printf("In disable_sleep_mode\n");

    bm.resume();

    batman.resume();
    dsman.resume();
}

// ****************************************************************************
//                             STANDBY MODE
// ****************************************************************************
 
static void _leader_standbymode_task(){

    uint32_t timeout_ms = STANDBY_TIMER_LSAT_STANDBY_PERIOD_MS;
    uint32_t timeout_us = timeout_ms * 1000;
  
    static bool modules_on = false;

    printf("PSU not plugged timeout: %d  ms \n",STANDBY_PSU_PLUGGED_TIMEOUT_MS);
    printf("WakeUp periode: %d  ms \n",timeout_ms);
    printf("Sleep Awake period: :%d  ms \n", (uint32_t) STANDBY_TIMER_LSAT_STAY_AWAKE_PERIOD_MS);
 
    printf("total timer time : %u  sec \n", (uint32_t) (standby_timer_elapsed_time_in_minutes()*60));
    printf("Total bias time  : %u  sec \n", (uint32_t) total_elapsed_bias_ms/1000);
    printf("Total elapsed time : %u  sec \n", (uint32_t) total_elapsed_time_sec);    
    printf("Total elapsed time : %.2f  min \n",_standby_get_total_elapsed_time_min()); 

    /*Disable modules when period to stay awake has overlapsed*/
    if(enable_sleep) {
        enable_sleep = false;

        bm.suspend();  
        dsman.suspend();
        sleep (1);

        modules_on = false;

        /*Enable sleep*/
        esp_sleep_enable_gpio_wakeup();
        esp_sleep_enable_timer_wakeup(timeout_us);
        esp_light_sleep_start();
        printf("exit sleep mode, it is time to work!\n");

        //Add as bias the sleep period
        _increment_elapsed_time_with_bias(timeout_ms);

        sleep(2);

        /*enable modules directly*/
        bm.resume();  
        dsman.resume();
        sleep (1);
        bm.enableAudio();

    }

 
    /*Check if the power button woke up system or timer timeout*/
    esp_sleep_source_t wake_up_cause = ESP_SLEEP_WAKEUP_UNDEFINED;
    wake_up_cause= esp_sleep_get_wakeup_cause();

    if(wake_up_cause == ESP_SLEEP_WAKEUP_GPIO){
        printf("External source cause wake up\n");

        wakeup_from_ext_source = true;
        standby_psu_unplugged_timeout = false;       
        return; 

    }
    else if(wake_up_cause == ESP_SLEEP_WAKEUP_TIMER){
        printf("Timer source cause wake up\n");

        /*Check if there is a new mode switch request for a period*/
        if(pic.newMode.take(pdMS_TO_TICKS(STANDBY_TIMER_PARSE_NEW_MODE_TIMEOUT_MS)))
        {
            printf("New mode received during wake up\n");
            sys.getCurrentSpkMode(&new_mode);

            /*New mode received from PIC*/
            if(new_mode != current_mode){

                //disable standby mode and process new mode
                wakeup_from_ext_source = true;
                standby_psu_unplugged_timeout = false;

                return; 
            }else{
                printf("No new mode received during wake up\n");
            }

        }else{
            wakeup_from_ext_source = false;

            /*Check if PSU timeout occured */           
            batman.determine_bq_state();    // check BQ24650
            batman.measure_soc();           // update internal SOC  
            batman.mode_of_rbuffer();       // returns a more stable SOC  
            batman.update_pic_soc();        // send state to PIC

            bool psu_plugged = batman.is_PSU_plugged();

            if (m_last_status_psu_plugged  != psu_plugged){
                printf("PSU status changed \n");
                m_last_status_psu_plugged = psu_plugged;

                /*Toggle the timer */
                if(standby_timer_is_enabled()){
                    if(!psu_plugged) standby_timer_reinit_no_auto_reload_mode();
                    else standby_timer_reinit_auto_reload_mode();
                }
            }

            if (psu_plugged){
                printf("PSU plugged in\n");

                //reset PSU timeout flags 
                standby_psu_unplugged_timeout = false;

                //sleep modules after period
                if(standy_timer_is_sleep_timeout_elapsed()){
                    enable_sleep = true;
                    standby_timer_clear_occurance();
                    _clear_elapsed_times();
                }
            }

            //Check if Auto power off is supported and if Power off timer overlapped
            else if(m_auto_pwr_off_enabled && _standby_get_total_elapsed_time_min() >= STANDBY_PSU_PLUGGED_TIMEOUT_MINUTES ) {

                standby_psu_unplugged_timeout = true;

                //Notify PIC that PSU unplugged timeout occured
                pic.sendPSUPluggedTimeout();      
            } else{
                /* Duration to stay awake occured?*/
                if(standy_timer_is_sleep_timeout_elapsed()){
                    enable_sleep = true;
                    standby_timer_clear_occurance();

                }
            }
        }        
    }
}

static void _follower_standbymode_task(){

    static uint32_t timeout_ms = STANDBY_TIMER_FSAT_STANDBY_PERIOD_MS;
    static uint32_t timeout_us = timeout_ms * 1000;

    printf("PSU not plugged timeout: %d  ms \n",STANDBY_PSU_PLUGGED_TIMEOUT_MS);
    printf("Sleep periode: %d  ms \n",timeout_ms);
    printf("total timer time : %u  sec \n", (uint32_t) (standby_timer_elapsed_time_in_minutes()*60));
    printf("Total bias time  : %u  sec \n", (uint32_t) total_elapsed_bias_ms/1000);
    printf("Total elapsed time : %u  sec \n", (uint32_t) total_elapsed_time_sec);    
    printf("Total elapsed time : %.2f  min \n",_standby_get_total_elapsed_time_min()); 


    /*Enable sleep afer stay awake has overlapsed*/
    if(enable_sleep) {
        enable_sleep = false;

        /*Enable sleep*/
        esp_sleep_enable_gpio_wakeup();
        esp_sleep_enable_timer_wakeup(timeout_us);
        esp_light_sleep_start();
        printf("exit sleep mode, it is time to work!\n");

        //Add as bias the sleep period
        _increment_elapsed_time_with_bias(timeout_ms);
    }  
     
    /*Check if the power button woke up system or timer timeout*/
    esp_sleep_source_t wake_up_cause = ESP_SLEEP_WAKEUP_UNDEFINED;
    wake_up_cause = esp_sleep_get_wakeup_cause();

    if(wake_up_cause == ESP_SLEEP_WAKEUP_GPIO){
        printf("External source cause wake up\n");

        wakeup_from_ext_source = true;
        standby_psu_unplugged_timeout = false;        
        return; 
    }
    else if(wake_up_cause == ESP_SLEEP_WAKEUP_TIMER){
        printf("Timer source cause wake up\n");
 
        /*Check if there is a new mode switch request for a period*/
        if(pic.newMode.take(pdMS_TO_TICKS(STANDBY_TIMER_PARSE_NEW_MODE_TIMEOUT_MS)))
        {
            printf("New mode received during wake up\n");
            sys.getCurrentSpkMode(&new_mode);

            /*New mode received from PIC*/
            if(new_mode != current_mode){

                //disable standby mode and process new mode
                wakeup_from_ext_source = true;
                standby_psu_unplugged_timeout = false;

                return; 
            } else{
                printf("No new mode received during wake up\n");
            }

        }else{
            wakeup_from_ext_source =false;

            /*Check if PSU timeout occured */           
            batman.determine_bq_state();    // check BQ24650
            batman.measure_soc();           // update internal SOC
            batman.mode_of_rbuffer();       // returns a more stable SOC
            batman.update_pic_soc();        // send state to PIC

            bool psu_plugged = batman.is_PSU_plugged();

            if (m_last_status_psu_plugged  != psu_plugged){
                printf("PSU status changed \n");
                m_last_status_psu_plugged = psu_plugged;

                /*Toggle the timer */
                if(standby_timer_is_enabled()){
                    if(!psu_plugged) standby_timer_reinit_no_auto_reload_mode();
                    else standby_timer_reinit_auto_reload_mode();
                }
            }

            if (psu_plugged){
                printf("PSU plugged in\n");

                //reset PSU timeout flags 
                standby_psu_unplugged_timeout = false;

                //sleep modules after period
                if(standy_timer_is_sleep_timeout_elapsed()){
                    enable_sleep = true;
                    standby_timer_clear_occurance();
                    _clear_elapsed_times();
                }                
            }

            //Check if Auto power off is supported and  if Power off timer overlapped
            else if( m_auto_pwr_off_enabled && _standby_get_total_elapsed_time_min() >= STANDBY_PSU_PLUGGED_TIMEOUT_MINUTES ) {

                standby_psu_unplugged_timeout = true;

                //Notify PIC that PSU unplugged timeout occured
                pic.sendPSUPluggedTimeout();       
            } else{
                /* Duration to stay awake occured?*/
                if(standy_timer_is_sleep_timeout_elapsed()){
                    enable_sleep = true;
                }
            }
        }        
    }
}


static void _enable_leader_standby_mode() {
    printf("Enabling leader standby mode\n");

    standbypmode = true;
    standby_mode = STANDBY_LEADER;
    enable_sleep = true;
    m_last_status_psu_plugged = batman.is_PSU_plugged();

    /*If PSU is plugged in, initialize timer with an alarm that does not increment, otherwise 
    increment it automatically*/
    timer_autoreload_t reload;

    if(m_last_status_psu_plugged) reload = TIMER_AUTORELOAD_EN;
    else reload = TIMER_AUTORELOAD_DIS;

    double m_timer_alarm_sec = STANDBY_TIMER_LSAT_STAY_AWAKE_PERIOD_MS/1000;
    standby_timer_task_start_up(&m_timer_alarm_sec, reload);

    bm.suspend();  
    dsman.suspend();

    /* give time  to properly disconnect a2dp sink before allowing the speaker to be 
     * awoken again */
    sleep(4);
    pic.sendStandbyMessage();
}

static void _enable_follower_standby_mode() {
    //TODO: give time  to properly disconnect a2dp sink before reabling audio
    printf("Enabling follower standby mode\n");
    
    standbypmode = true;
    standby_mode = STANDBY_FOLLOWER;
    m_last_status_psu_plugged = batman.is_PSU_plugged();
    enable_sleep = true;

    /*If PSU is plugged in, initialize timer with an alarm that does not increment, otherwise 
    increment it automatically*/
    timer_autoreload_t reload;
    
    if(m_last_status_psu_plugged) reload = TIMER_AUTORELOAD_EN;
    else reload = TIMER_AUTORELOAD_DIS;

    double m_timer_alarm_sec = STANDBY_TIMER_DEFAULT_STAY_AWAKE_PERIOD_MS/1000;
    standby_timer_task_start_up(&m_timer_alarm_sec, reload);
    
    dsman.suspend();
    bm.suspend();

    sleep(2);
    pic.sendStandbyMessage();
}

static void _disable_standby_mode() {
    printf("In disable_standby_mode\n");

    bm.resume();  
    dsman.resume();

    sleep(2);

    pic.requestSpeakerMode();
    standbypmode = false;   
    standby_mode = NO_STANDBY;

    standby_timer_task_shut_down();
    _clear_elapsed_times();

    printf("Recovery from standbymode\n");


}

static void _disable_follower_standby_mode(){
    printf(" Disable follower standby mode\n");

    _clear_elapsed_times();
}

// *****************************************************************************
// *****************************************************************************
//                                MAIN FUNCTION
// *****************************************************************************
// *****************************************************************************

extern "C" void app_main() {
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    hal.setup();

    isOtaBtClassic = false;

    bm.init_dm();
 
    //spp must be called after BluemanSat::init_dm()
    btc_bm.init_spp();
    batman.start();
    pic.start();
    dsman.start();

    LIS2DE12 accel;
    acc_status = accel.ping();
    pic.sendAccelStatus(acc_status);

    new_mode = SpkMode::BOOT;
    current_mode = SpkMode::BOOT;

    // This request initializes the ESP after it crashes. During a normal boot
    // the PIC will send this info after initializing each mode.
    pic.requestSpeakerMode();

    SpkMode temp_prev_mode = current_mode; //new

    while (true) {
        printf("\n\n");
        printf("temp_prev_mode %c:\n", (char) temp_prev_mode);
        printf("current_mode %c:\n", (char) current_mode);
        printf("new_mode %c:\n", (char) new_mode);


        //wait for new mode when module gets new mode from PIC or ESP was woken up from sleep from PIC
        if(current_mode != SpkMode::STANDBY || (current_mode == SpkMode::STANDBY &&wakeup_from_ext_source)){
            printf("mode is not standby. take()\n");
            pic.newMode.take();

            sys.getCurrentSpkMode(&new_mode);
           

            if(wakeup_from_ext_source) {
                //reset count 
                wakeup_from_ext_source = false;  //reset variable
            }

        }
        else{
            printf("mode is standby. \n");

            if(standby_mode == STANDBY_LEADER) _leader_standbymode_task();
            else if(standby_mode == STANDBY_FOLLOWER)  _follower_standbymode_task();
        }

        if(current_mode == new_mode)
        {
            if (new_mode == SpkMode::SLEEP)
                LOGW("MAIN", "Enabling sleep mode");        

            continue;
        }else{
            printf("new mode recieved\n");
     
            temp_prev_mode = current_mode; //new 
            current_mode = new_mode;

            switch(temp_prev_mode)
            {
                case SpkMode::SLEEP: _disable_sleep_mode(); break; 
                case SpkMode::LEADER: _disable_leader_mode(); break; 
                case SpkMode::FOLLOWER: _disable_follower_mode(); break;
                case SpkMode::STANDBY: _disable_standby_mode(); break;
                case SpkMode::BOOT: break;
                default:
                    LOGW("MAIN", "Trying to change mode with UNKNOWN previous mode");
            }

            switch(new_mode)
            {
                case SpkMode::SLEEP:
                    if(temp_prev_mode != SpkMode::BOOT) 
                       _enable_sleep_mode();
                    break; 
                case SpkMode::LEADER: 
                    _enable_leader_mode(); break; 
                case SpkMode::FOLLOWER: 
                    _enable_follower_mode(); break;
                case SpkMode::STANDBY:
                    if(temp_prev_mode==SpkMode::LEADER) _enable_leader_standby_mode();
                    else if(temp_prev_mode==SpkMode::FOLLOWER) _enable_follower_standby_mode();
                     break;
                case SpkMode::BOOT: 
                    LOGE("MAIN", "Trying to change mode to BOOT run-time");
                    break;
                default:
                    LOGW("MAIN", "Trying to change mode with UNKNOWN new mode");
                    current_mode = temp_prev_mode;
            }
       }
    }
}




