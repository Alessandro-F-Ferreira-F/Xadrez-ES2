#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

typedef uint64_t time_ns;

time_ns timer_now_ns(void);

static inline double timer_ns_to_ms(time_ns ns) { return (double)ns / 1e6; }
static inline double timer_ns_to_s(time_ns ns) { return (double)ns / 1e9; }


#endif /* TIMER_H */