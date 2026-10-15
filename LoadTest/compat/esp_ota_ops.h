#pragma once

#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include "esp_err.h"

typedef uint32_t esp_ota_handle_t;

typedef struct esp_partition_t {
    uint32_t address;
    uint32_t size;
} esp_partition_t;

#define ESP_PARTITION_TYPE_APP      0x00
#define ESP_PARTITION_SUBTYPE_ANY   0xFF

static inline const esp_partition_t *esp_ota_get_next_update_partition(
    const esp_partition_t *start_from)
{
    (void)start_from;
    static const esp_partition_t dummy = {0};
    return &dummy;
}

static inline esp_err_t esp_ota_begin(
    const esp_partition_t *partition,
    size_t image_size,
    esp_ota_handle_t *out_handle)
{
    (void)partition;
    (void)image_size;
    if (out_handle) *out_handle = 1;
    return ESP_OK;
}

static inline esp_err_t esp_ota_write(
    esp_ota_handle_t handle,
    const void *data,
    size_t size)
{
    (void)handle;
    (void)data;
    (void)size;
    return ESP_OK;
}

static inline esp_err_t esp_ota_end(esp_ota_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

static inline esp_err_t esp_ota_set_boot_partition(
    const esp_partition_t *partition)
{
    (void)partition;
    return ESP_OK;
}

#define ESP_APP_DESC_MAGIC_WORD 0xABCD5432

typedef struct {
    uint32_t magic_word;
    uint32_t secure_version;
    uint32_t reserv1[2];
    char version[32];
    char project_name[32];
    char time[16];
    char date[16];
    char idf_ver[32];
    uint8_t app_elf_sha256[32];
    uint16_t min_efuse_blk_rev_full;
    uint16_t max_efuse_blk_rev_full;
    uint8_t mmu_page_size;
    uint8_t reserv3[3];
    uint32_t reserv2[18];
} esp_app_desc_t;

static inline esp_err_t esp_ota_get_partition_description(
    const esp_partition_t *partition,
    esp_app_desc_t *app_desc)
{
    (void)partition;

    if (!app_desc)
        return ESP_ERR_INVALID_ARG;

    memset(app_desc, 0, sizeof(*app_desc));
    app_desc->magic_word = ESP_APP_DESC_MAGIC_WORD;

    return ESP_OK;
}
static inline esp_err_t esp_partition_erase_range(
    const esp_partition_t *partition,
    size_t offset,
    size_t size)
{
    (void)partition;
    (void)offset;
    (void)size;
    return ESP_OK;
}