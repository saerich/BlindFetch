#ifndef COMPAT_ESP_WIFI_H
#define COMPAT_ESP_WIFI_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sodium.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA
} wifi_mode_t;

typedef enum {
    WIFI_IF_STA = 0,
    WIFI_IF_AP
} wifi_interface_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WEP,
    WIFI_AUTH_WPA_PSK,
    WIFI_AUTH_WPA2_PSK,
    WIFI_AUTH_WPA_WPA2_PSK,
    WIFI_AUTH_WPA2_ENTERPRISE,
    WIFI_AUTH_WPA3_PSK
} wifi_auth_mode_t;

typedef enum {
    WIFI_PS_NONE = 0,
    WIFI_PS_MIN_MODEM,
    WIFI_PS_MAX_MODEM
} wifi_ps_type_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    uint8_t scan_method;
    bool bssid_set;
    uint8_t bssid[6];
    uint8_t channel;
    uint16_t listen_interval;
} wifi_sta_config_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    uint8_t ssid_len;
    uint8_t channel;
    wifi_auth_mode_t authmode;
    uint8_t max_connection;
} wifi_ap_config_t;

typedef union {
    wifi_sta_config_t sta;
    wifi_ap_config_t ap;
} wifi_config_t;

typedef struct {
    int dummy;
} wifi_init_config_t;

#define WIFI_INIT_CONFIG_DEFAULT() ((wifi_init_config_t){0})

extern uint8_t compat_wifi_mac[6];

static inline esp_err_t esp_wifi_set_mac(
    wifi_interface_t interface,
    const uint8_t mac[6])
{
    (void)interface;

    if (!mac) return ESP_ERR_INVALID_ARG;

    memcpy(compat_wifi_mac, mac, 6);
    return ESP_OK;
}

esp_err_t esp_wifi_get_mac(wifi_interface_t interface, uint8_t mac[6]);
esp_err_t esp_wifi_init(const wifi_init_config_t *config);

static inline esp_err_t esp_wifi_deinit(void)
{
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_mode(wifi_mode_t mode)
{
    (void)mode;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_start(void)
{
    return ESP_OK;
}

static inline esp_err_t esp_wifi_stop(void)
{
    return ESP_OK;
}

static inline esp_err_t esp_wifi_connect(void)
{
    return ESP_OK;
}

static inline esp_err_t esp_wifi_disconnect(void)
{
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *config)
{
    (void)interface;
    (void)config;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_ps(wifi_ps_type_t type)
{
    (void)type;
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif

#endif