#include "logger.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// Global logger instance
Logger* g_logger = NULL;
static pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;

// ANSI color codes
static const char* COLOR_RESET = "\033[0m";

// Simple path expansion for ~
static char* expand_path(const char* path) {
    if (!path || path[0] != '~') {
        return strdup(path);
    }

    const char* home = getenv("HOME");
    if (!home) {
        return strdup(path); // Fallback to original path
    }

    size_t home_len = strlen(home);
    size_t path_len = strlen(path);
    char* expanded = (char*)malloc(home_len + path_len); // path includes ~ which we replace

    if (!expanded) {
        return NULL;
    }

    strcpy(expanded, home);
    strcpy(expanded + home_len, path + 1); // Skip the ~

    return expanded;
}

static const char* level_names[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "CRITICAL"
};

static const char* level_colors[] = {
    "\033[36m",  // COLOR_DEBUG - Cyan
    "\033[32m",  // COLOR_INFO - Green
    "\033[33m",  // COLOR_WARNING - Yellow
    "\033[31m",  // COLOR_ERROR - Red
    "\033[35m"   // COLOR_CRITICAL - Magenta
};

int logger_init(LogLevel level, const char* filename, int use_colors) {
    pthread_mutex_lock(&logger_mutex);

    if (g_logger) {
        logger_shutdown();
    }

    g_logger = (Logger*)malloc(sizeof(Logger));
    if (!g_logger) {
        pthread_mutex_unlock(&logger_mutex);
        return -1;
    }

    g_logger->level = level;
    g_logger->use_colors = use_colors && isatty(fileno(stderr));
    g_logger->include_timestamp = 1;
    g_logger->prefix = NULL;

    if (filename) {
        char* expanded_filename = expand_path(filename);
        if (!expanded_filename) {
            free(g_logger);
            g_logger = NULL;
            pthread_mutex_unlock(&logger_mutex);
            return -1;
        }

        g_logger->file = fopen(expanded_filename, "a");
        free(expanded_filename);

        if (!g_logger->file) {
            free(g_logger);
            g_logger = NULL;
            pthread_mutex_unlock(&logger_mutex);
            return -1;
        }
    } else {
        g_logger->file = stderr;
    }

    LOG_INFO("Logger initialized with level %s", level_names[level]);

    pthread_mutex_unlock(&logger_mutex);
    return 0;
}

void logger_shutdown(void) {
    pthread_mutex_lock(&logger_mutex);

    if (g_logger) {
        if (g_logger->file && g_logger->file != stderr) {
            fclose(g_logger->file);
        }
        free(g_logger->prefix);
        free(g_logger);
        g_logger = NULL;
    }

    pthread_mutex_unlock(&logger_mutex);
}

void logger_set_level(LogLevel level) {
    pthread_mutex_lock(&logger_mutex);
    if (g_logger) {
        g_logger->level = level;
    }
    pthread_mutex_unlock(&logger_mutex);
}

void logger_log(LogLevel level, const char* file, int line, const char* func, const char* format, ...) {
    pthread_mutex_lock(&logger_mutex);

    if (!g_logger || level < g_logger->level) {
        pthread_mutex_unlock(&logger_mutex);
        return;
    }

    FILE* output = g_logger->file ? g_logger->file : stderr;

    // Timestamp
    if (g_logger->include_timestamp) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(output, "[%s] ", timestamp);
    }

    // Level with color
    if (g_logger->use_colors) {
        fprintf(output, "%s", level_colors[level]);
    }
    fprintf(output, "[%s]", level_names[level]);
    if (g_logger->use_colors) {
        fprintf(output, "%s", COLOR_RESET);
    }

    // Prefix
    if (g_logger->prefix) {
        fprintf(output, " %s", g_logger->prefix);
    }

    // Location
    fprintf(output, " %s:%d:%s() - ", file, line, func);

    // Message
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);

    fprintf(output, "\n");
    fflush(output);

    pthread_mutex_unlock(&logger_mutex);
}