#include "logger.h"

void log_message(const char* level, const char* format, ...) {
    fprintf(stderr, "[%s] ", level);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

void redirect_stderr(void) {
    int fd = open("/var/log/xwm.log", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd != -1) {
        if (fd != STDERR_FILENO) {
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
    } else {
        ERROR("Cannot open log file '/var/log/xwm.log'\n");
    }
}
