#include "esp_wifi.h"
#include <sodium.h>
#include <stdint.h>
#include <string.h>

uint8_t compat_wifi_mac[6];
static uint8_t device_mac[6];
static int mac_initialized = 0;

esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{
    (void)config;
    randombytes_buf(compat_wifi_mac, sizeof(compat_wifi_mac));
    compat_wifi_mac[0] = (compat_wifi_mac[0] | 0x02) & 0xFE;
    return ESP_OK;
}

esp_err_t esp_efuse_mac_get_default(uint8_t *mac)
{
    if (!mac) { return ESP_ERR_INVALID_ARG; }
    memcpy(mac, compat_wifi_mac, sizeof(compat_wifi_mac));
    return ESP_OK;
}

esp_err_t esp_wifi_get_mac(wifi_interface_t interface, uint8_t *mac)
{
    (void)interface;
    if (!mac) { return ESP_ERR_INVALID_ARG; }
    esp_wifi_init(NULL);
    memcpy(mac, compat_wifi_mac, 6);
    return ESP_OK;
}