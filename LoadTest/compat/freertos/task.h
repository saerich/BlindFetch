#pragma once

#include <stdint.h>
#include <time.h>

#define pdMS_TO_TICKS(ms) (ms)

static inline void vTaskDelay(uint32_t ticks)
{
    struct timespec ts = {
        .tv_sec = ticks / 1000,
        .tv_nsec = (long)(ticks % 1000) * 1000000L
    };

    nanosleep(&ts, NULL);
}