#include "standby_timer.h"

#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"


#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE    (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_DEFAULT_INTERVAL_SEC   (STANDBY_TIMER_LSAT_STAY_AWAKE_PERIOD_MS/1000) // sample test interval for the first timer
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload


double m_timer_alarm_sec = TIMER_DEFAULT_INTERVAL_SEC;
bool m_timer_enabled = false;


/*
 * A sample structure to pass events
 * from the timer interrupt handler to the main program.
 */
typedef struct {
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

xQueueHandle timer_queue;


uint32_t num_of_occurances = 0;
bool timer_elapsed = false;
uint64_t last_counter_value = 0;


/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
static void inline print_timer_counter(uint64_t counter_value)
{
    printf("Counter: 0x%08x%08x\n", (uint32_t) (counter_value >> 32),
           (uint32_t) (counter_value));
    printf("Time   : %.8f s\n", (double) counter_value / TIMER_SCALE);
}


/*
 * Timer group0 ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 */
void IRAM_ATTR timer_group0_isr(void *para)
{
    timer_spinlock_take(TIMER_GROUP_0);
    int timer_idx = (int) para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t timer_intr = timer_group_get_intr_status_in_isr(TIMER_GROUP_0);
    uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(TIMER_GROUP_0, timer_idx);

    /* Prepare basic event data
       that will be then sent back to the main program task */
    timer_event_t evt;
    evt.timer_group = 0;
    evt.timer_idx = timer_idx;
    evt.timer_counter_value = timer_counter_value;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if (timer_intr & TIMER_INTR_T0) {
        // evt.type = TEST_WITHOUT_RELOAD;
        evt.type = TIMER_AUTORELOAD_DIS;
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
        if (m_timer_alarm_sec >0)
        {
            timer_counter_value += (uint64_t) (m_timer_alarm_sec * TIMER_SCALE);
        }
        else{
            timer_counter_value += (uint64_t) (TIMER_DEFAULT_INTERVAL_SEC * TIMER_SCALE);
        }
        
        timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, timer_idx, timer_counter_value);
    } else if (timer_intr & TIMER_INTR_T1) {
        // evt.type = TEST_WITH_RELOAD;
        evt.type = TIMER_AUTORELOAD_EN;
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_1);
    } else {
        evt.type = -1; // not supported even type
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, timer_idx);

    /* Now just send the event data back to the main program task */
    xQueueSendFromISR(timer_queue, &evt, NULL);
    timer_spinlock_give(TIMER_GROUP_0);
}


/*
 * The main task of this example program
 */
static void _timer_evt_task(void *arg)
{
    while (1) {
        timer_event_t evt;
        xQueueReceive(timer_queue, &evt, portMAX_DELAY);

        /* Print information that the timer reported an event */
        if (evt.type == TEST_WITHOUT_RELOAD) {
            // if (evt.type == TIMER_AUTORELOAD_DIS) {

            num_of_occurances++;
            timer_elapsed = true;

        } else if (evt.type == TEST_WITH_RELOAD) {
            // } else if (evt.type == TIMER_AUTORELOAD_EN) {
            timer_elapsed = true;
        } else {
            printf("\n    UNKNOWN EVENT TYPE\n");
        }

        /* Print the timer values as visible by this task */
        // printf("-------- TASK TIME --------\n");
        uint64_t task_counter_value;
        timer_get_counter_value(evt.timer_group, evt.timer_idx, &task_counter_value);
        last_counter_value = task_counter_value;       
    }
}


/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
static void _tg0_timer_init(int timer_idx,
                                   bool auto_reload, double timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
                       (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
}


/*
 * Reinitialize selected timer of the timer group 
 *
 * timer_group - timer group to initialize
 * timer_idx - the timer number to initialize
 * alarm_time - time in seconds to configure the timer alarm
 * reload - reload type to use. Either increament counter after alarm or reset to pre-configured default value
 */ 
static void _tg_timer_reinit(int timer_group, int timer_idx, double alarm_time, timer_autoreload_t reload){

    last_counter_value =0;

    timer_set_auto_reload(timer_group,timer_idx,reload);  
    timer_pause(timer_group, timer_idx);
    timer_set_counter_value(timer_group, timer_idx, 0x0);
    timer_set_alarm_value(timer_group, timer_idx, alarm_time * TIMER_SCALE);
    timer_enable_intr(timer_group, timer_idx);
    timer_isr_register(timer_group, timer_idx, timer_group0_isr,
                       (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(timer_group, timer_idx);

}

static bool _is_auto_reload_mode(){

    timer_config_t timer_conf = {0};

    esp_err_t res = timer_get_config(TIMER_GROUP_0, TIMER_0, &timer_conf);

    return (timer_conf.auto_reload == TIMER_AUTORELOAD_EN);

}

void standby_timer_reinit_no_auto_reload_mode(void){

    _tg_timer_reinit(TIMER_GROUP_0, TIMER_0,m_timer_alarm_sec,TIMER_AUTORELOAD_DIS );
}

void standby_timer_reinit_auto_reload_mode(void){

    _tg_timer_reinit(TIMER_GROUP_0, TIMER_0,m_timer_alarm_sec,TIMER_AUTORELOAD_EN );      
}

bool standby_timer_task_start_up(double* timer_alarm_sec, timer_autoreload_t reload){  

    if(m_timer_enabled) {
        printf("Warning, timer already enabled\n");
        return false; 
    }

    last_counter_value = 0;

	num_of_occurances = 0;
	standby_timer_clear_occurance();

	if(!timer_alarm_sec) m_timer_alarm_sec = (double) TIMER_DEFAULT_INTERVAL_SEC; 
	else m_timer_alarm_sec = (double) *timer_alarm_sec;


	timer_queue = xQueueCreate(10, sizeof(timer_event_t));
	_tg0_timer_init(TIMER_0, reload, m_timer_alarm_sec);
	xTaskCreate(_timer_evt_task, "timer_evt_task", 2048, NULL, 5, NULL);

    m_timer_enabled = true;
    return true;
}

bool standby_timer_task_shut_down(void) {
 
    if(!m_timer_enabled)  {
        printf("Warning, timer already disabled\n");
        return false; 
    }

    last_counter_value = 0;

    if (timer_queue) {
        vQueueDelete(timer_queue);
        timer_queue = NULL;
    }

    timer_disable_intr(TIMER_GROUP_0, TIMER_0);
    timer_deinit(TIMER_GROUP_0, TIMER_0);

    standby_timer_clear_occurance();

    m_timer_enabled = false;

    return true;
}

uint32_t standby_timer_get_num_occurances(void){
	return num_of_occurances;
}


bool standy_timer_is_sleep_timeout_elapsed (void){
	return timer_elapsed;
}

void standby_timer_clear_occurance(void){
	timer_elapsed = false;
}

double standby_timer_elapsed_time_in_minutes(void){
    printf("Elapsed time: %.2f minutes\n", (double) last_counter_value / TIMER_SCALE / 60);
	return (double) last_counter_value / TIMER_SCALE / 60; 
}

bool standby_timer_is_enabled(void){
    return m_timer_enabled;
}