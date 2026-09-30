#ifndef LOG_H
#define LOG_H


void log_emit(const char *level, const char *file, int line,
              const char *func, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 5, 6)))
#endif
    ;

#define LOG_ERROR(...) log_emit("ERROR", __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef DEBUG
#  define LOG_DEBUG(...) log_emit("DEBUG", __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#  define LOG_DEBUG(...) ((void)0)
#endif

#endif /* LOG_H */
