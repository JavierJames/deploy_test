#ifndef __BT_APP_CORE_H__
#define __BT_APP_CORE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_a2dp_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* event for handler "bt_av_hdl_stack_up */
enum {
    BT_APP_EVT_STACK_UP = 0,
    BT_APP_EVT_CONNECTABLE_DISCOVERABLE,
    BT_APP_EVT_CONNECTABLE,
    BT_APP_EVT_STACK_DOWN,
    BT_APP_EVT_DISCONNECT
};

/**
 * @brief     handler for the dispatched work
 */
typedef void (*bt_app_cb_t)(uint16_t event, void* param);

/* message to be sent */
typedef struct {
    uint16_t sig;   /*!< signal to bt_app_task */
    uint16_t event; /*!< message event id */
    bt_app_cb_t cb; /*!< context switch callback */
    void* param;    /*!< parameter area needs to be last */
} bt_app_msg_t;

/**
 * @brief     parameter deep-copy function to be customized
 */
typedef void (*bt_app_copy_cb_t)(bt_app_msg_t* msg, void* p_dest, void* p_src);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     reconnect to last connected device
 */
bool bt_app_a2d_reconnect_last_dev();

/**
 * @brief update status of the current connection.
 */
void bt_av_update_conn_status(struct a2d_conn_stat_param* conn);

/**
 * @brief get status of the current connection.
 */
esp_a2d_connection_state_t bt_av_get_conn_status();

/**
 * @brief     work dispatcher for the application task
 */
bool bt_app_work_dispatch(bt_app_cb_t p_cback,
                          uint16_t event,
                          void* p_params,
                          int param_len,
                          bt_app_copy_cb_t p_copy_cback);

/**
 * @brief     dispatcher for stack events
 */
void bt_av_hdl_stack_evt(uint16_t event, void* p_param);

/**
 * @brief     prepare environment (task handler & event queue)
 */
void bt_app_task_start_up(xSemaphoreHandle link_evt);

/**
 * @brief     stop environment (task handler & event queue)
 */
void bt_app_task_shut_down(void);

#ifdef __cplusplus
}
#endif

#endif /* __BT_APP_CORE_H__ */
