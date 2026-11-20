#define _POSIX_C_SOURCE 199309L

#include "timer.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif




time_ns timer_now_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER ticks, frequency;
    QueryPerformanceCounter(&ticks);
    QueryPerformanceFrequency(&frequency);
    uint64_t value = (uint64_t)ticks.QuadPart;
    uint64_t hz = (uint64_t)frequency.QuadPart;
    return (value / hz) * 1000000000ULL + (value % hz) * 1000000000ULL / hz;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}
