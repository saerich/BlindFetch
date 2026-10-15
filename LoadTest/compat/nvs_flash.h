#ifndef COMPAT_NVS_FLASH_H
#define COMPAT_NVS_FLASH_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_ERR_NVS_BASE             0x1100
#define ESP_ERR_NVS_NOT_INITIALIZED  (ESP_ERR_NVS_BASE + 0x01)
#define ESP_ERR_NVS_NOT_FOUND        (ESP_ERR_NVS_BASE + 0x02)
#define ESP_ERR_NVS_NO_FREE_PAGES    (ESP_ERR_NVS_BASE + 0x0D)
#define ESP_ERR_NVS_NEW_VERSION_FOUND (ESP_ERR_NVS_BASE + 0x10)

static inline esp_err_t nvs_flash_init(void)
{
    return ESP_OK;
}

static inline esp_err_t nvs_flash_erase(void)
{
    return ESP_OK;
}

static inline esp_err_t nvs_flash_deinit(void)
{
    return ESP_OK;
}

static inline esp_err_t nvs_flash_init_partition(
    const char *partition_label)
{
    (void)partition_label;
    return ESP_OK;
}

static inline esp_err_t nvs_flash_erase_partition(
    const char *partition_label)
{
    (void)partition_label;
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif

#endif