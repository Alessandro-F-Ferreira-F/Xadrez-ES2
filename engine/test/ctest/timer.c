#define _POSIX_C_SOURCE 199309L

#include "timer.h"
#include <time.h>




time_ns timer_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}



// windows implementation...