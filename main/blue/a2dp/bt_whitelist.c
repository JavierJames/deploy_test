#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "esp_gap_bt_api.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "bt_whitelist.h"

static const char* BT_WHITELIST_TAG = "BT_WHITELIST";

uint8_t whitelist_size = 0;

esp_bd_addr_t a2dp_whitelist [A2DP_WHITELIST_SIZE];
esp_bd_addr_t a2dp_whitelist_interm [A2DP_WHITELIST_SIZE];

bt_whitelist_flags_t bt_whitelist_flags;

nvs_handle handle;

void bt_whitelist_clear() 
{
    ESP_LOGI(BT_WHITELIST_TAG, "Clearing whitelist");

    int num_of_devs = esp_bt_gap_get_bond_device_num();

    esp_bd_addr_t devs[num_of_devs];
    esp_err_t err;

    whitelist_size = 0;

    ESP_LOGI(BT_WHITELIST_TAG, "Amount of paired devices: %d", num_of_devs);

    if (num_of_devs < 1) {
        ESP_LOGI(BT_WHITELIST_TAG, "No devices in whitelist");        
        return;                                                       
    }

    esp_bt_gap_get_bond_device_list(&num_of_devs, devs);


    ESP_LOGI(BT_WHITELIST_TAG, "Removing bonds");
    for(int i = 0; i < num_of_devs; i++)
    {
        ESP_LOGI(BT_WHITELIST_TAG,"Removing bond for: %02x:%02x:%02x:%02x:%02x:%02x", ESP_BD_ADDR_HEX(devs[i]));
        err = esp_bt_gap_remove_bond_device(devs[i]);
        if(err)
            ESP_LOGE(BT_WHITELIST_TAG,"Failed to remove bond");
    }

    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    err = nvs_erase_key(handle, "whitelist");
    if(err)
        ESP_LOGE(BT_WHITELIST_TAG, "Could not erase whitelist (%s)", esp_err_to_name(err));
        
    err = nvs_erase_key(handle, "bt_wl_flags");
    if(err)
        ESP_LOGE(BT_WHITELIST_TAG, "Could not erase flags (%s)", esp_err_to_name(err));

    ESP_LOGI(BT_WHITELIST_TAG, "Clearing done");
}

uint8_t bt_whitelist_size()
{
    if(whitelist_size < 1)
        ESP_LOGI(BT_WHITELIST_TAG, "No devices in whitelist");
    return whitelist_size;  
}

void bt_whitelist_last(esp_bd_addr_t bda_o)
{
    for(uint8_t i = 0; i < ESP_BD_ADDR_LEN; i++)
        bda_o[i] = a2dp_whitelist[0][i];

    ESP_LOGI(BT_WHITELIST_TAG,"last bda: %02x:%02x:%02x:%02x:%02x:%02x", ESP_BD_ADDR_HEX(bda_o));
}

void bt_whitelist_rearrange(uint8_t idx)
{
    for(uint8_t i = idx; i > 0; i--)
    {
        if(i >= A2DP_WHITELIST_SIZE)
            continue;

        memcpy(a2dp_whitelist[i], a2dp_whitelist[i - 1], ESP_BD_ADDR_LEN);
    }
}

void bt_whitelist_log()
{
    for (uint8_t i=0; i < whitelist_size; i++)
        ESP_LOGI(BT_WHITELIST_TAG, "%d) %02x:%02x:%02x:%02x:%02x:%02x",i,ESP_BD_ADDR_HEX(a2dp_whitelist[i]));
}

bool bt_whitelist_compare_bda(esp_bd_addr_t bda1, esp_bd_addr_t bda2)
{
    for(uint8_t i=0; i<ESP_BD_ADDR_LEN; i++)
        if(bda1[i] != bda2[i])
            return false;

    return true;
}

bool bt_whitelist_notify_connection(esp_bd_addr_t new_bda)
{
    ESP_LOGI(BT_WHITELIST_TAG, "New connection %02x:%02x:%02x:%02x:%02x:%02x", ESP_BD_ADDR_HEX(new_bda));

    /* check if bda exists in whitelist, if so, get index */
    int8_t in_wl_idx = -1;
    for (uint8_t i=0; i < whitelist_size; i++)
        if(bt_whitelist_compare_bda(new_bda, a2dp_whitelist[i]))
            in_wl_idx = i;

    if (in_wl_idx != -1) {
        /* found in wl, put at top */
        ESP_LOGI(BT_WHITELIST_TAG, "MAC address found in whitelist");
        bt_whitelist_rearrange(in_wl_idx);
    } else {
        /* not found in wl, add to top */
        ESP_LOGI(BT_WHITELIST_TAG, "Connected as new MAC address");
        bt_whitelist_rearrange(whitelist_size);

        if(whitelist_size < A2DP_WHITELIST_SIZE)
            whitelist_size++;

        ESP_LOGI(BT_WHITELIST_TAG, "Whitelist size: %d", whitelist_size);
    }

    /* add new entry to idx 0 */
    memcpy(a2dp_whitelist[0], new_bda, ESP_BD_ADDR_LEN);   

    /* save changes to flash */
    bt_whitelist_nvs_save();

    return (in_wl_idx != -1);
}

void bt_whitelist_nvs_load()
{
    size_t len;
    char nvs_whitelist_str[(A2DP_WHITELIST_SIZE * MAC_ADDR_STR_SIZE) + 1];

    ESP_LOGI(BT_WHITELIST_TAG, "Loading from NVS");

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    err = nvs_get_str(handle, "whitelist", nvs_whitelist_str, &len);
    if (err != ESP_OK) {
        if(err == ESP_ERR_NVS_NOT_FOUND)
            ESP_LOGW(BT_WHITELIST_TAG, "NVS key not found, could be empty\n");
        else 
            ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) fetching data!\n", esp_err_to_name(err));
        return;
    }

    if(len < 1)
    {
        ESP_LOGI(BT_WHITELIST_TAG, "Nothing loaded, whitelist is empty");
        return;
    }

    ESP_LOGI(BT_WHITELIST_TAG, "Loaded whitelist string: %s", nvs_whitelist_str);

    whitelist_size = (len - 1) / MAC_ADDR_STR_SIZE;
    
    esp_bd_addr_t addr;
    for (uint8_t i = 0; i < whitelist_size; i++) {
        for (uint8_t j = 0; j < MAC_ADDR_STR_SIZE; j+=2) {
            char hex[2];
            hex[0] = nvs_whitelist_str[j + i * MAC_ADDR_STR_SIZE];
            hex[1] = nvs_whitelist_str[j + i * MAC_ADDR_STR_SIZE + 1];

            addr[j>>1] = (uint8_t) strtol(hex, 0, 16);
        }
        
        memcpy(a2dp_whitelist[i], addr, ESP_BD_ADDR_LEN);
    }

    /* load flags */
    uint8_t flags;
    err = nvs_get_u8(handle, "bt_wl_flags", &flags);
    if (err != ESP_OK) {
        if(err == ESP_ERR_NVS_NOT_FOUND)
            ESP_LOGW(BT_WHITELIST_TAG, "NVS flags key not found, could be empty\n");
        else 
            ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) fetching flags!\n", esp_err_to_name(err));
        return;
    }
    
    /* extract flags */
    bt_whitelist_flags.should_reconnect = flags & 1;
    ESP_LOGI(BT_WHITELIST_TAG, "Should reconnect: (%s)", (bt_whitelist_flags.should_reconnect ? "true" : "false"));

}

void bt_whitelist_nvs_save()
{
    if(whitelist_size < 1)
        return;

    char nvs_whitelist_str[(A2DP_WHITELIST_SIZE * MAC_ADDR_STR_SIZE) + 1];

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    /* convert a2dp_addr array to str array */
    for(uint8_t i = 0; i < whitelist_size; i++)
    {
        esp_bd_addr_t a;
        memcpy(a, a2dp_whitelist[i], ESP_BD_ADDR_LEN);

        char bda_str[MAC_ADDR_STR_SIZE + 1];
        snprintf(bda_str, MAC_ADDR_STR_SIZE + 1, "%02x%02x%02x%02x%02x%02x",
                a[0],a[1],a[2],a[3],a[4],a[5]); 

        if(i == 0)
            strcpy(nvs_whitelist_str, bda_str);
        else
            strcat(nvs_whitelist_str, bda_str);
    }

    strcat(nvs_whitelist_str, "\0");

    err = nvs_set_str(handle, "whitelist", nvs_whitelist_str);
    if (err != ESP_OK) {
        ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) writing!\n", esp_err_to_name(err));
        return;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) committing to NVS handle!\n", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(BT_WHITELIST_TAG, "Saved whitelist string: %s", nvs_whitelist_str);

    nvs_close(handle);
}

void bt_whitelist_save_flags()
{
    uint8_t flags = 0;
    flags |=  bt_whitelist_flags.should_reconnect << 0; 

    ESP_LOGI(BT_WHITELIST_TAG, "Saving flags to NVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u8(handle, "bt_wl_flags", flags);
    if (err != ESP_OK) {
        ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) writing!\n", esp_err_to_name(err));
        return;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(BT_WHITELIST_TAG, "Error (%s) committing to NVS handle!\n", esp_err_to_name(err));
        return;
    }

    nvs_close(handle);
}

bt_whitelist_flags_t* bt_whitelist_get_flags()
{
    return &bt_whitelist_flags;
}
