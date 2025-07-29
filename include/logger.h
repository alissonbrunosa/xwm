#ifndef WM_LOGGER_H
#define WM_LOGGER_H

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define INFO(format, ...)  log_message("INFO",  format, ##__VA_ARGS__)
#define DEBUG(format, ...) log_message("DEBUG", format, ##__VA_ARGS__)
#define ERROR(format, ...) log_message("ERROR", format, ##__VA_ARGS__)

#define FATAL(message, ...)                    \
    do {                                       \
        log_message("FATAL", message, __VA_ARGS__); \
        exit(1);                               \
    } while (0)


void log_message(const char* level, const char* format, ...);
void redirect_stderr(void);

#endif // WM_LOGGER_H
