#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_CRITICAL = 4
} LogLevel;

// Logger configuration
typedef struct {
    LogLevel level;
    FILE* file;
    int use_colors;
    int include_timestamp;
    char* prefix;
} Logger;

// Global logger instance
extern Logger* g_logger;

/**
 * @brief Initialize the global logger
 *
 * @param level Minimum log level to output
 * @param filename Log file path (NULL for stderr)
 * @param use_colors Whether to use ANSI color codes
 * @return 0 on success, -1 on failure
 */
int logger_init(LogLevel level, const char* filename, int use_colors);

/**
 * @brief Shutdown the global logger
 */
void logger_shutdown(void);

/**
 * @brief Set the minimum log level
 */
void logger_set_level(LogLevel level);

/**
 * @brief Log a message with specified level
 */
void logger_log(LogLevel level, const char* file, int line, const char* func, const char* format, ...);

/**
 * @brief Convenience macros for different log levels
 */
#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...) logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARNING(...) logger_log(LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_CRITICAL(...) logger_log(LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

/**
 * @brief Assert with logging
 */
#define LOG_ASSERT(condition, ...) \
    do { \
        if (!(condition)) { \
            LOG_CRITICAL("Assertion failed: " #condition " - " __VA_ARGS__); \
            abort(); \
        } \
    } while (0)

#endif // LOGGER_H