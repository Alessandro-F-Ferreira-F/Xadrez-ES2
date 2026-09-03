#ifndef CHESS_ENGINE_LOG_H
#define CHESS_ENGINE_LOG_H

#include <stdio.h>
#include <stdlib.h>

void log_msg(const char *msg, const char *file, int line, const char *func);

#define LOG_ERROR(msg) log_msg((msg), __FILE__, __LINE__, __func__)

#endif /* CHESS_ENGINE_LOG_H */
