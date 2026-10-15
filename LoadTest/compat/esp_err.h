#ifndef COMPAT_ESP_ERR_H
#define COMPAT_ESP_ERR_H

#include <stdint.h>
#include <stdio.h>

typedef int32_t esp_err_t;

#define ESP_OK                    0
#define ESP_FAIL                 -1
#define ESP_ERR_NO_MEM           0x101
#define ESP_ERR_INVALID_ARG      0x102
#define ESP_ERR_INVALID_STATE    0x103
#define ESP_ERR_INVALID_SIZE     0x104
#define ESP_ERR_NOT_FOUND        0x105
#define ESP_ERR_NOT_SUPPORTED    0x106
#define ESP_ERR_TIMEOUT          0x107
#define ESP_ERR_INVALID_RESPONSE 0x108
#define ESP_ERR_HW_CRYPTO_BASE   0xC000

static inline const char *esp_err_to_name(esp_err_t err)
{
    switch (err) {
        case ESP_OK:                    return "ESP_OK";
        case ESP_FAIL:                  return "ESP_FAIL";
        case ESP_ERR_NO_MEM:            return "ESP_ERR_NO_MEM";
        case ESP_ERR_INVALID_ARG:       return "ESP_ERR_INVALID_ARG";
        case ESP_ERR_INVALID_STATE:     return "ESP_ERR_INVALID_STATE";
        case ESP_ERR_INVALID_SIZE:      return "ESP_ERR_INVALID_SIZE";
        case ESP_ERR_NOT_FOUND:         return "ESP_ERR_NOT_FOUND";
        case ESP_ERR_NOT_SUPPORTED:     return "ESP_ERR_NOT_SUPPORTED";
        case ESP_ERR_TIMEOUT:           return "ESP_ERR_TIMEOUT";
        case ESP_ERR_INVALID_RESPONSE:  return "ESP_ERR_INVALID_RESPONSE";
        default:                        return "ESP_ERR_UNKNOWN";
    }
}
#ifndef ESP_ERROR_CHECK_WITHOUT_ABORT
#define ESP_ERROR_CHECK_WITHOUT_ABORT(x) do {               \
    esp_err_t err = (x);                                   \
    if (err != ESP_OK) {                                   \
        fprintf(stderr, "%s:%d: ESP error: %d\n",          \
                __FILE__, __LINE__, (int)err);             \
    }                                                      \
} while (0)
#endif

#endif
