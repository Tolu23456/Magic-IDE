#ifndef CONFIG_H
#define CONFIG_H

#include <json-c/json.h>
#include <stdbool.h>

// Configuration structure
typedef struct {
    // UI settings
    int window_width;
    int window_height;
    bool window_maximized;
    char* theme;

    // Editor settings
    char* font_family;
    int font_size;
    bool word_wrap;
    bool show_line_numbers;
    int tab_size;
    bool insert_spaces;

    // LSP settings
    bool enable_lsp;
    char* lsp_servers_path;

    // Terminal settings
    char* shell;
    char* terminal_font;
    int terminal_font_size;

    // Git settings
    bool enable_git;
    char* git_path;

    // Plugin settings
    bool enable_plugins;
    char* plugins_path;

    // Logging
    char* log_level;
    char* log_file;
    bool log_colors;

    // Raw JSON object for custom settings
    struct json_object* custom_settings;
} Config;

/**
 * @brief Initialize configuration system
 *
 * @return 0 on success, -1 on failure
 */
int config_init(void);

/**
 * @brief Load configuration from file
 *
 * @param filename Configuration file path
 * @return 0 on success, -1 on failure
 */
int config_load(const char* filename);

/**
 * @brief Save configuration to file
 *
 * @param filename Configuration file path
 * @return 0 on success, -1 on failure
 */
int config_save(const char* filename);

/**
 * @brief Get configuration instance
 *
 * @return Pointer to global config, NULL if not initialized
 */
Config* config_get(void);

/**
 * @brief Set a configuration value
 *
 * @param key Configuration key (dot-separated for nested)
 * @param value JSON value to set
 * @return 0 on success, -1 on failure
 */
int config_set(const char* key, struct json_object* value);

/**
 * @brief Get a configuration value
 *
 * @param key Configuration key (dot-separated for nested)
 * @return JSON object, NULL if not found
 */
struct json_object* config_get_value(const char* key);

/**
 * @brief Get string value from config
 *
 * @param key Configuration key
 * @param default_value Default value if not found
 * @return String value or default
 */
const char* config_get_string(const char* key, const char* default_value);

/**
 * @brief Get integer value from config
 *
 * @param key Configuration key
 * @param default_value Default value if not found
 * @return Integer value or default
 */
int config_get_int(const char* key, int default_value);

/**
 * @brief Get boolean value from config
 *
 * @param key Configuration key
 * @param default_value Default value if not found
 * @return Boolean value or default
 */
bool config_get_bool(const char* key, bool default_value);

/**
 * @brief Cleanup configuration system
 */
void config_cleanup(void);

#endif // CONFIG_H