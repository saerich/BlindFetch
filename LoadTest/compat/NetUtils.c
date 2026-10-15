#include "NetUtils.h"

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_mac.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static bool IsWiFiConnected = false;

esp_err_t NetworkNVSInit(void)
{
    return ESP_OK;
}

esp_err_t GetDeviceMAC(uint8_t *macAddress)
{
    if (!macAddress)
        return ESP_ERR_INVALID_ARG;

    return esp_efuse_mac_get_default(macAddress);
}

esp_err_t GetDeviceMACAsString(char *macAddress)
{
    if (!macAddress)
        return ESP_ERR_INVALID_ARG;

    uint8_t mac[6];

    esp_err_t err = GetDeviceMAC(mac);
    if (err != ESP_OK)
        return err;

    sprintf(macAddress,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2],
        mac[3], mac[4], mac[5]);

    return ESP_OK;
}

esp_err_t WiFiConnect(const char *ssid, const char *pwd)
{
    (void)ssid;
    (void)pwd;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK)
        return err;

    IsWiFiConnected = true;

    return ESP_OK;
}

bool WiFiSync(int timeoutMS)
{
    (void)timeoutMS;
    return IsWiFiConnected;
}

bool WifiConnected(void)
{
    return IsWiFiConnected;
}