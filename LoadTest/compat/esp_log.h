#ifndef ESP_LOG_H
#define ESP_LOG_H

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_INVALID_RESPONSE 0x108
#define ESP_ERR_NOT_SUPPORTED 0x106
#define ESP_ERR_HW_CRYPTO_BASE 0xC000

#define ESP_ERROR_CHECK(x) do { \
    esp_err_t err = (x);        \
    if (err != ESP_OK) abort(); \
} while (0)

#define ESP_LOGE(tag, fmt, ...) \
    fprintf(stderr, "E (%s): " fmt "\n", tag, ##__VA_ARGS__)

#define ESP_LOGW(tag, fmt, ...) \
    fprintf(stderr, "W (%s): " fmt "\n", tag, ##__VA_ARGS__)

#define ESP_LOGI(tag, fmt, ...) \
    fprintf(stdout, "I (%s): " fmt "\n", tag, ##__VA_ARGS__)

#define ESP_LOGD(tag, fmt, ...) \
    fprintf(stdout, "D (%s): " fmt "\n", tag, ##__VA_ARGS__)

#define ESP_LOGV(tag, fmt, ...) \
    fprintf(stdout, "V (%s): " fmt "\n", tag, ##__VA_ARGS__)
#endif