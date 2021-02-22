#ifndef __BT_APP_AV_H__
#define __BT_APP_AV_H__

#include <stdint.h>
#include <unistd.h>
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief     callback function for A2DP sink
 */
void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* param);

/**
 * @brief     callback function for A2DP sink audio data stream
 */
void bt_app_a2d_data_cb(uint8_t* data, uint32_t len);

/**
 * @brief     callback function for AVRCP controller
 */
void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t* param);

/**
 * @brief     set signal lost mode
 */
void bt_a2d_set_signal_lost_mode(bool mode);

/**
 * @brief     set leader mode enabled
 */
void bt_a2d_set_leader_mode_enabled(bool mode);

/**
 * @brief     get leader mode enabled
 */
bool bt_a2d_get_leader_mode_enabled();

/**
 * @brief     disconnected, and deinit a2dp sink 
 */
void bt_a2d_close();

#ifdef __cplusplus
}
#endif

#endif /* __BT_APP_AV_H__*/
