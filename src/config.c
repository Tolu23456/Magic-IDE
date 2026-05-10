#include "config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>

// Global configuration instance
static Config* g_config = NULL;

// Default configuration values
static const char* DEFAULT_CONFIG =
"{"
"  \"ui\": {"
"    \"window_width\": 1200,"
"    \"window_height\": 800,"
"    \"window_maximized\": false,"
"    \"theme\": \"default\""
"  },"
"  \"editor\": {"
"    \"font_family\": \"Monaco, 'Courier New', monospace\","
"    \"font_size\": 14,"
"    \"word_wrap\": true,"
"    \"show_line_numbers\": true,"
"    \"tab_size\": 4,"
"    \"insert_spaces\": true"
"  },"
"  \"lsp\": {"
"    \"enable_lsp\": true,"
"    \"lsp_servers_path\": \"~/.magic-ide/lsp-servers\""
"  },"
"  \"terminal\": {"
"    \"shell\": \"/bin/bash\","
"    \"terminal_font\": \"Monaco, 'Courier New', monospace\","
"    \"terminal_font_size\": 12"
"  },"
"  \"git\": {"
"    \"enable_git\": true,"
"    \"git_path\": \"/usr/bin/git\""
"  },"
"  \"plugins\": {"
"    \"enable_plugins\": true,"
"    \"plugins_path\": \"~/.magic-ide/plugins\""
"  },"
"  \"logging\": {"
"    \"log_level\": \"info\","
"    \"log_file\": \"~/.magic-ide/magic-ide.log\","
"    \"log_colors\": true"
"  }"
"}";

static char* expand_path(const char* path) {
    if (!path || path[0] != '~') {
        return strdup(path);
    }

    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }

    if (!home) {
        LOG_ERROR("Could not determine home directory");
        return strdup(path);
    }

    size_t home_len = strlen(home);
    size_t path_len = strlen(path);
    char* expanded = (char*)malloc(home_len + path_len); // path includes ~ which we replace

    if (!expanded) {
        LOG_ERROR("Failed to allocate memory for path expansion");
        return NULL;
    }

    strcpy(expanded, home);
    strcpy(expanded + home_len, path + 1); // Skip the ~

    return expanded;
}

static int create_config_directory(const char* config_file) {
    char* dir_path = strdup(config_file);
    if (!dir_path) return -1;

    // Find the last directory separator
    char* last_sep = strrchr(dir_path, '/');
    if (last_sep) {
        *last_sep = '\0';

        // Create directory if it doesn't exist
        struct stat st;
        if (stat(dir_path, &st) == -1) {
            if (mkdir(dir_path, 0755) == -1) {
                LOG_ERROR("Failed to create config directory: %s", dir_path);
                free(dir_path);
                return -1;
            }
        }
    }

    free(dir_path);
    return 0;
}

int config_init(void) {
    if (g_config) {
        LOG_WARNING("Config already initialized");
        return 0;
    }

    g_config = (Config*)malloc(sizeof(Config));
    if (!g_config) {
        LOG_ERROR("Failed to allocate memory for config");
        return -1;
    }

    memset(g_config, 0, sizeof(Config));

    // Load default configuration
    struct json_object* default_config = json_tokener_parse(DEFAULT_CONFIG);
    if (!default_config) {
        LOG_ERROR("Failed to parse default configuration");
        free(g_config);
        g_config = NULL;
        return -1;
    }

    // Set default values
    g_config->custom_settings = default_config;

    LOG_INFO("Configuration system initialized");
    return 0;
}

int config_load(const char* filename) {
    if (!g_config) {
        LOG_ERROR("Config not initialized");
        return -1;
    }

    char* expanded_filename = expand_path(filename);
    if (!expanded_filename) {
        return -1;
    }

    FILE* file = fopen(expanded_filename, "r");
    if (!file) {
        LOG_INFO("Config file not found, using defaults: %s", expanded_filename);
        free(expanded_filename);
        return 0; // Not an error, just use defaults
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        LOG_ERROR("Failed to allocate memory for config file");
        fclose(file);
        free(expanded_filename);
        return -1;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    struct json_object* loaded_config = json_tokener_parse(buffer);
    free(buffer);

    if (!loaded_config) {
        LOG_ERROR("Failed to parse config file: %s", expanded_filename);
        free(expanded_filename);
        return -1;
    }

    // Merge with defaults (loaded config takes precedence)
    if (g_config->custom_settings) {
        json_object_put(g_config->custom_settings);
    }
    g_config->custom_settings = loaded_config;

    free(expanded_filename);
    LOG_INFO("Configuration loaded from: %s", filename);
    return 0;
}

int config_save(const char* filename) {
    if (!g_config || !g_config->custom_settings) {
        LOG_ERROR("Config not initialized or empty");
        return -1;
    }

    char* expanded_filename = expand_path(filename);
    if (!expanded_filename) {
        return -1;
    }

    if (create_config_directory(expanded_filename) != 0) {
        free(expanded_filename);
        return -1;
    }

    FILE* file = fopen(expanded_filename, "w");
    if (!file) {
        LOG_ERROR("Failed to open config file for writing: %s", expanded_filename);
        free(expanded_filename);
        return -1;
    }

    const char* json_str = json_object_to_json_string_ext(g_config->custom_settings, JSON_C_TO_STRING_PRETTY);
    if (!json_str) {
        LOG_ERROR("Failed to serialize config to JSON");
        fclose(file);
        free(expanded_filename);
        return -1;
    }

    fprintf(file, "%s\n", json_str);
    fclose(file);

    free(expanded_filename);
    LOG_INFO("Configuration saved to: %s", filename);
    return 0;
}

Config* config_get(void) {
    return g_config;
}

struct json_object* config_get_value(const char* key) {
    if (!g_config || !g_config->custom_settings || !key) {
        return NULL;
    }

    struct json_object* current = g_config->custom_settings;
    char* key_copy = strdup(key);
    if (!key_copy) {
        return NULL;
    }

    char* token = strtok(key_copy, ".");
    while (token && current) {
        struct json_object* next = NULL;
        if (!json_object_object_get_ex(current, token, &next)) {
            current = NULL;
            break;
        }

        current = next;
        token = strtok(NULL, ".");
    }

    free(key_copy);
    return current;
}

int config_set(const char* key, struct json_object* value) {
    if (!g_config || !g_config->custom_settings || !key || !value) {
        return -1;
    }

    char* key_copy = strdup(key);
    if (!key_copy) {
        return -1;
    }

    char* token = strtok(key_copy, ".");
    struct json_object* current = g_config->custom_settings;
    char* next_token = NULL;

    while (token) {
        next_token = strtok(NULL, ".");
        if (!next_token) {
            json_object_object_add(current, token, json_object_get(value));
            break;
        }

        struct json_object* child = NULL;
        if (!json_object_object_get_ex(current, token, &child) ||
            json_object_get_type(child) != json_type_object) {
            child = json_object_new_object();
            json_object_object_add(current, token, child);
        }

        current = child;
        token = next_token;
    }

    free(key_copy);
    return 0;
}

const char* config_get_string(const char* key, const char* default_value) {
    struct json_object* value = config_get_value(key);
    if (value && json_object_get_type(value) == json_type_string) {
        return json_object_get_string(value);
    }
    return default_value;
}

int config_get_int(const char* key, int default_value) {
    struct json_object* value = config_get_value(key);
    if (value && json_object_get_type(value) == json_type_int) {
        return json_object_get_int(value);
    }
    return default_value;
}

bool config_get_bool(const char* key, bool default_value) {
    struct json_object* value = config_get_value(key);
    if (value && json_object_get_type(value) == json_type_boolean) {
        return json_object_get_boolean(value);
    }
    return default_value;
}

void config_cleanup(void) {
    if (g_config) {
        if (g_config->custom_settings) {
            json_object_put(g_config->custom_settings);
        }
        free(g_config);
        g_config = NULL;
    }
    LOG_INFO("Configuration system cleaned up");
}