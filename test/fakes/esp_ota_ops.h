#ifndef _OTA_OPS_H
#define _OTA_OPS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_SIZE_UNKNOWN \
    0xffffffff /*!< Used for esp_ota_begin() if new image size is unknown */

// Frop "esp_partition.h"
typedef struct {
    int address;
    int subtype;
} esp_partition_t;
const esp_partition_t part_fake{0, 0};

// From "esp_err.h"
#define ESP_OK 0
#define ESP_FAIL -1
typedef int32_t esp_err_t;

typedef uint32_t esp_ota_handle_t;

esp_err_t esp_ota_begin(const esp_partition_t* partition,
                        size_t image_size,
                        esp_ota_handle_t* out_handle) {
    return ESP_OK;
};

esp_err_t esp_ota_write(esp_ota_handle_t handle,
                        const void* data,
                        size_t size) {
    return ESP_OK;
};

esp_err_t esp_ota_end(esp_ota_handle_t handle) {
    return ESP_OK;
};

esp_err_t esp_ota_set_boot_partition(const esp_partition_t* partition) {
    return ESP_OK;
};

const esp_partition_t* esp_ota_get_boot_partition(void) {
    return &part_fake;
};

const esp_partition_t* esp_ota_get_running_partition(void) {
    return &part_fake;
};

const esp_partition_t* esp_ota_get_next_update_partition(
    const esp_partition_t* start_from) {
    return &part_fake;
};

// From "esp_system.h"
void esp_restart(void) {
    return;
};

#ifdef __cplusplus
}
#endif

#endif /* OTA_OPS_H */
