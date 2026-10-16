#include <ssot.h>
#include <esp_log.h>
#include <esp_err.h>
#include <NetUtils.h>
#include <NetworkedOPAQUE.h>
#include <BlindFetch.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include <esp_task.h>

#ifndef WIFI_SSID
#error "WIFI_SSID must be specified in .env"
#endif

#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD must be specified in .env"
#endif

#ifndef OPAQUE_PASSWORD
#error "OPAQUE_PASSWORD must be specified in .env"
#endif

#ifndef OPAQUE_SERVER_URL
#error "OPAQUE_SERVER_URL must be specified in .env"
#endif

const char* wifiSSID = WIFI_SSID;
const char* wifiPassword = WIFI_PASSWORD;
const char* opaquePassword = OPAQUE_PASSWORD;
const char* opaqueServerURL = OPAQUE_SERVER_URL;
//const char* deviceFirmwareKey = FIRMWARE_KEY; // Under normal circumstances, this would be an EFUSE burned value from a fabricator. 
const char* TAG = "[Main]";

static const char* firmwareKeys[] = {
    FIRMWARE_KEY_BBT01,
    FIRMWARE_KEY_LB01,
    FIRMWARE_KEY_PSC01,
    FIRMWARE_KEY_SLM01
};
static const char *firmwareNames[] =
{
    "BBT01",
    "LB01",
    "PSC01",
    "SLM01"
};

#define FIRMWARE_COUNT (sizeof(firmwareKeys) / sizeof(firmwareKeys[0]))

static void fisherYatesShuffle(size_t* indicies, size_t count)
{
    for(size_t i = count - 1; i > 0; --i)
    {
        size_t j = esp_random() % (i+1);
        size_t temp = indicies[i];
        indicies[i] = indicies[j];
        indicies[j] = temp;
    }
}

/**
 * @brief Primary entry point for the ESPIDF application, the intent of this application is to provide a one-shot example of
 * an implementation of a fully oblivious over the air updating mechanism, running on clock speed 160MHz
 * @author Darren Nevin
 */
void app_main()
{    
    #ifdef Wait
        vTaskDelay(pdMS_TO_TICKS(5000));
    #endif
    int64_t startTime = esp_timer_get_time();
    WiFiConnect(wifiSSID, wifiPassword);

    char macAddr[18];
    int64_t macAddrTime = esp_timer_get_time();
    GetDeviceMACAsString(macAddr);
    int64_t macAddrEndTime = esp_timer_get_time();
    
    WiFiSync(10000);
    
    #ifndef PlainOTA 
        int64_t opaqueInitTime = esp_timer_get_time();
        OPAQUEInit(macAddr);
        int64_t opaqueInitEndTime = esp_timer_get_time();
        int64_t opaqueRegisterStartTime = esp_timer_get_time();
        esp_err_t err = NetworkedOPAQUERegister(opaqueServerURL, macAddr, opaquePassword);
        int64_t opaqueRegisterEndTime = esp_timer_get_time();
    #endif
    int count = 25;
    for(int run = 0; run < count; ++run)
    {
        size_t firmwareOrder[FIRMWARE_COUNT];
        for(size_t i = 0; i < FIRMWARE_COUNT; ++i) { firmwareOrder[i] = i; }
        fisherYatesShuffle(firmwareOrder, FIRMWARE_COUNT);

        for(size_t exec = 0; exec < FIRMWARE_COUNT; ++exec)
        {
            #ifndef PlainOTA
                size_t firmwareIdx = firmwareOrder[exec];
                const char* deviceFirmwareKey = firmwareKeys[firmwareIdx];
                const char* selectedFirmwareName = firmwareNames[firmwareIdx];
                #ifdef ReportUsage
                    size_t heap_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
                    size_t heap_low_before = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
                #endif
                if(err == ESP_ERR_NOT_SUPPORTED) { ESP_LOGI(TAG, "User is already registered"); }
                uint8_t skClient[OPAQUE_SHARED_SECRETBYTES];
                uint8_t exportKey[crypto_hash_sha512_BYTES];

                int64_t opaqueLoginTime = esp_timer_get_time();
                ESP_ERROR_CHECK(NetworkedOPAQUELogin(opaqueServerURL, macAddr, opaquePassword, skClient, exportKey));
                int64_t opaqueLoginEndTime = esp_timer_get_time();

                #ifdef HEADERONLY
                    OTAHeaderGeneration(opaqueServerURL, deviceFirmwareKey, macAddr, skClient);
                    continue;
                #endif
                
                int64_t downloadStartTime = esp_timer_get_time();
                BlindDownloadFirmware(opaqueServerURL, deviceFirmwareKey, macAddr, skClient);
                int64_t downloadEndTime = esp_timer_get_time();

                #ifdef ReportUsage
                    size_t heap_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
                    size_t heap_low_after = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);

                    size_t peak_heap_used = heap_before - heap_low_after;
                    size_t lowest_free_heap = heap_low_after;
                    size_t heap_high = 0;
                    if(heap_low_after < heap_low_before) { heap_high = heap_before - heap_low_after; }
                    else { heap_high = peak_heap_used; }
                    size_t heap_low = heap_low_after;
                    
                    ESP_LOGI("OTA", "Sending metrics to the Server");           
                    #ifdef Wait
                        vTaskDelay(pdMS_TO_TICKS(5000));
                    #endif     
                    SendMetricsToServer(opaqueServerURL, heap_high, heap_low, opaqueLoginEndTime, downloadEndTime, selectedFirmwareName);
                    ESP_LOGI("TIMING", "Peak Heap: %d, Lowest Heap: %d, Execution Time: %d", heap_after, lowest_free_heap, downloadEndTime - opaqueLoginEndTime);
                #endif
                ESP_LOGI("OTA", "OTA Completed, %lld - %lld", opaqueLoginTime, downloadEndTime);            
            #else
                uint64_t startTime = esp_timer_get_time();
                uint64_t endTime = PlainOTADownload(opaqueServerURL);
                if(endTime == ESP_FAIL) { ESP_LOGE("OTA", "Failed to OTA."); continue; }
                ESP_LOGI("OTA", "OTA Completed, %lld - %lld", startTime, endTime);
            #endif
        }
    }
}
