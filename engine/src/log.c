#include "log.h"

#include <stdarg.h>
#include <stdio.h>


void log_emit(const char *level, const char *file, int line,
              const char *func, const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "[%s] %s:%d %s(): ", level, file, line, func);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);
    fflush(stderr);
}
