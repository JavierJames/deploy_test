#ifndef __SYSTEM_TIMER_H__
#define __SYSTEM_TIMER_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// #include <components/soc/include/hal/timer_types.h>
#include "esp_timer.h"
#include "esp_sleep.h"

#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

	
#define STANDBY_TIMER_LSAT_STAY_AWAKE_PERIOD_MS  3000   //Time to stay awake when awoken to make a BT connection and enable modules
#define STANDBY_TIMER_LSAT_STANDBY_PERIOD_MS   500	    //Duration to put the module in standby mode
#define STANDBY_TIMER_FSAT_STANDBY_PERIOD_MS  30000 	//Duration to put the module in standby mode
#define STANDBY_TIMER_PARSE_NEW_MODE_TIMEOUT_MS 3000  	//Duration to wait for new PIC mode when in standby mode 
#define STANDBY_TIMER_DEFAULT_STAY_AWAKE_PERIOD_MS 1000 //Default period in ms to keep this module awake during standby

/**
 * @brief     start timer for the shutdown timeout when in standby
  *timer_alarm_sec time in seconds to periodically trigger the alarm 
  *reload type an alarm to use. TIMER_AUTORELOAD_DIS automatically increases the counter 
  after every alarm, while TIMER_AUTORELOAD_EN resets it to a pre-configured value
 */
bool standby_timer_task_start_up(double* timer_alarm_sec, timer_autoreload_t reload);


/**
 * @brief     stop timer for the shutdown timeout when in standby
 */
bool standby_timer_task_shut_down(void);

/**
* @brief     Check to see if the standby sleep period has elapsed. Return true if yes; false otherwise
*/
bool standy_timer_is_sleep_timeout_elapsed (void);

/**
* @brief     Get the number of occurances that the timer has elapsed
*/
uint32_t standby_timer_get_num_occurances(void);

/**
* @brief     Reset the number of occurances that the timer has elapsed
*/
void standby_timer_clear_occurance(void);

/**
* @brief     Get the total time in minutes that the timer has run
*/
double standby_timer_elapsed_time_in_minutes(void);

/**
* @brief     Check to see if the timer is enabled
*/
bool standby_timer_is_enabled(void);

/**
* @brief     Reinitialize the timer module with auto reload on. Must first use standby_timer_task_start_up() once before. 
	         Use this function if it is desired that the time counter resets after every alarm perid in seconds.
*/
void standby_timer_reinit_auto_reload_mode(void );

/**
* @brief     Reinitialize the timer module with auto reload off. Must first use standby_timer_task_start_up() once before. 
	         Use this function if it is desired that the time counter increments after every alarm perid in seconds.
*/
void standby_timer_reinit_no_auto_reload_mode(void);


#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_TIMER_H__ */
