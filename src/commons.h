#ifndef SC_COMMONS_H
#define SC_COMMONS_H

// ---- logging ----
typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
} log_level_t;

void flog(log_level_t level, const char* format, ...);
#endif