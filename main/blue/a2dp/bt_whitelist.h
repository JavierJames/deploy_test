#ifndef __BT_WHITELIST_H__
#define __BT_WHITELIST_H__

#include <stdint.h>
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

typedef struct  
{
    bool should_reconnect; /* @brief reconnect after sleep/boot to last wl entry */

} bt_whitelist_flags_t;

#ifdef __cplusplus
extern "C" {
#endif
 
#define A2DP_WHITELIST_SIZE 6

/* length of mac addr as str, excluding \0 */
#define MAC_ADDR_STR_SIZE (A2DP_WHITELIST_SIZE * 2)

/**
 * @brief forget the whitelisted devices
 */
void bt_whitelist_clear();

/**
 * @brief get size of whitelist
 * @return size 
 */
uint8_t bt_whitelist_size();

/**
 * @brief rearrange whitelist until given index
 * frees up idx 0 for new entry
 */
void bt_whitelist_rearrange(uint8_t);

/**
 * @brief print bt whitelist
 */
void bt_whitelist_log();

/**
 * @brief get last connected device in whitelist
 * @param out esp_bd_addr_t to last connected esp_bd_addr_t
 */
void bt_whitelist_last(esp_bd_addr_t);

/**
 * @brief compare two bt addresses
 * @return true is equal
 */
bool bt_whitelist_compare_bda(esp_bd_addr_t, esp_bd_addr_t);

/**
 * @brief adds to whitelist, if it does, moves it to the top
 * otherwise adds it to the top
 * @param a2d 
 * @return true if exists in whitelist
 */
bool bt_whitelist_notify_connection(esp_bd_addr_t);

/**
 * @brief load whitelist from nvs
 */
void bt_whitelist_nvs_load();

/**
 * @brief save whitelist to nvs
 */
void bt_whitelist_nvs_save();

/**
 * @brief save flags
 */
void bt_whitelist_save_flags();

/**
 * @brief get ptr to flags
 */
bt_whitelist_flags_t* bt_whitelist_get_flags();

#ifdef __cplusplus
}
#endif

#endif /* __BT_WHITELIST_H__ */
