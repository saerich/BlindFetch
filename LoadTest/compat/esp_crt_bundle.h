#ifndef COMPAT_ESP_CRT_BUNDLE_H
#define COMPAT_ESP_CRT_BUNDLE_H

#include "esp_err.h"

static inline esp_err_t esp_crt_bundle_attach(void *conf)
{
    (void)conf;
    return ESP_OK;
}

#endif