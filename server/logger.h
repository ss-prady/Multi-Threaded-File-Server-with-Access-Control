#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <pthread.h>

// Log levels
typedef enum {
    LOG_INFO,
    LOG_ERROR,
    LOG_DEBUG
} LogLevel;

// Functions
int init_logger(const char *filename);
void close_logger();
void log_message(LogLevel level, const char *format, ...);

#endif // LOGGER_H
