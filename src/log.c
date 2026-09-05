#include "log.h"


void log_msg(const char *msg, const char *file, int line, const char *func) {
    fprintf(stderr, "error: %s [%s] [%d] [%s]\n", msg, file, line, func);
}