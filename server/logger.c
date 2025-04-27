#include "logger.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex;

int init_logger(const char *filename) {
    log_file = fopen(filename, "a"); // append mode
    if (!log_file) {
        perror("Failed to open log file");
        return -1;
    }
    pthread_mutex_init(&log_mutex, NULL);
    return 0;
}

void close_logger() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_destroy(&log_mutex);
}

static const char *level_to_string(LogLevel level) {
    switch (level) {
        case LOG_INFO: return "INFO";
        case LOG_ERROR: return "ERROR";
        case LOG_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}

void log_message(LogLevel level, const char *format, ...) {
    if (!log_file) return; // logger not initialized

    pthread_mutex_lock(&log_mutex);

    // Timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    // Log level string
    const char *level_str = level_to_string(level);

    // Write to file
    fprintf(log_file, "[%s] [%s] ", timestamp, level_str);

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file); // Immediately flush to file

    pthread_mutex_unlock(&log_mutex);
}
