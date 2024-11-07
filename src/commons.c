#include "commons.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void flog(log_level_t level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    // print time
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    printf("[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    switch (level) {
        case LOG_INFO:
            printf("[INFO] ");
            break;
        case LOG_WARNING:
            printf("[WARNING] ");
            break;
        case LOG_ERROR:
            printf("[ERROR] ");
            break;
    }

    vprintf(format, args);
    va_end(args);
}