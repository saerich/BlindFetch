#ifndef COMPAT_ESP_MAC_H
#define COMPAT_ESP_MAC_H

#include <stdint.h>
#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP_MAC_WIFI_STA = 0,
    ESP_MAC_WIFI_SOFTAP,
    ESP_MAC_BT,
    ESP_MAC_ETH,
    ESP_MAC_IEEE802154,
    ESP_MAC_BASE,
    ESP_MAC_EFUSE_FACTORY
} esp_mac_type_t;

static inline esp_err_t esp_read_mac(
    uint8_t *mac,
    esp_mac_type_t type)
{
    (void)type;

    if (!mac)
        return ESP_ERR_INVALID_ARG;

    return esp_wifi_get_mac(WIFI_IF_STA, mac);
}

static inline esp_err_t esp_efuse_mac_get_default(
    uint8_t *mac)
{
    return esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY);
}

static inline esp_err_t esp_base_mac_addr_get(
    uint8_t *mac)
{
    return esp_read_mac(mac, ESP_MAC_BASE);
}

static inline esp_err_t esp_base_mac_addr_set(
    const uint8_t *mac)
{
    return esp_wifi_set_mac(WIFI_IF_STA, mac);
}

#ifdef __cplusplus
}
#endif

#endif