#include "plugin_manager.h"
#include "logger.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>

// Plugin manager structure
struct PluginManager {
    char* plugins_dir;
    PluginInfo** plugins;
    size_t plugin_count;
    size_t plugin_capacity;
};

// Plugin manifest filename
#define PLUGIN_MANIFEST "plugin.json"

// Plugin library filename
#define PLUGIN_LIBRARY "plugin.so"

static int plugin_info_load_from_manifest(PluginInfo* info, const char* manifest_path) {
    FILE* file = fopen(manifest_path, "r");
    if (!file) {
        LOG_ERROR("Failed to open plugin manifest: %s", manifest_path);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        LOG_ERROR("Failed to allocate buffer for plugin manifest");
        fclose(file);
        return -1;
    }

    size_t bytes_read = fread(buffer, 1, size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    struct json_object* manifest = json_tokener_parse(buffer);
    free(buffer);

    if (!manifest) {
        LOG_ERROR("Failed to parse plugin manifest JSON: %s", manifest_path);
        return -1;
    }

    // Extract basic information
    struct json_object* obj;

    if (json_object_object_get_ex(manifest, "id", &obj)) {
        info->id = strdup(json_object_get_string(obj));
    }

    if (json_object_object_get_ex(manifest, "name", &obj)) {
        info->name = strdup(json_object_get_string(obj));
    }

    if (json_object_object_get_ex(manifest, "version", &obj)) {
        info->version = strdup(json_object_get_string(obj));
    }

    if (json_object_object_get_ex(manifest, "description", &obj)) {
        info->description = strdup(json_object_get_string(obj));
    }

    if (json_object_object_get_ex(manifest, "author", &obj)) {
        info->author = strdup(json_object_get_string(obj));
    }

    // Extract dependencies
    if (json_object_object_get_ex(manifest, "dependencies", &obj) &&
        json_object_get_type(obj) == json_type_array) {
        info->dependency_count = json_object_array_length(obj);
        info->dependencies = (char**)malloc(sizeof(char*) * info->dependency_count);

        for (size_t i = 0; i < info->dependency_count; i++) {
            struct json_object* dep = json_object_array_get_idx(obj, i);
            info->dependencies[i] = strdup(json_object_get_string(dep));
        }
    }

    json_object_put(manifest);
    return 0;
}

static void plugin_info_free(PluginInfo* info) {
    if (!info) return;

    free(info->id);
    free(info->name);
    free(info->version);
    free(info->description);
    free(info->author);
    free(info->path);

    if (info->dependencies) {
        for (size_t i = 0; i < info->dependency_count; i++) {
            free(info->dependencies[i]);
        }
        free(info->dependencies);
    }

    if (info->handle) {
        dlclose(info->handle);
    }

    free(info);
}

static int plugin_load_library(PluginInfo* info) {
    char lib_path[PATH_MAX];
    snprintf(lib_path, sizeof(lib_path), "%s/%s", info->path, PLUGIN_LIBRARY);

    info->handle = dlopen(lib_path, RTLD_LAZY);
    if (!info->handle) {
        LOG_ERROR("Failed to load plugin library %s: %s", lib_path, dlerror());
        return -1;
    }

    // Get plugin API
    PluginAPI* api = (PluginAPI*)dlsym(info->handle, "plugin_api");
    if (!api) {
        LOG_ERROR("Plugin %s does not export plugin_api symbol", info->id);
        dlclose(info->handle);
        info->handle = NULL;
        return -1;
    }

    // Check API version
    if (strcmp(api->api_version, "1.0") != 0) {
        LOG_ERROR("Plugin %s has incompatible API version: %s", info->id, api->api_version);
        dlclose(info->handle);
        info->handle = NULL;
        return -1;
    }

    // Initialize plugin
    if (api->init && api->init() != 0) {
        LOG_ERROR("Plugin %s initialization failed", info->id);
        dlclose(info->handle);
        info->handle = NULL;
        return -1;
    }

    LOG_INFO("Plugin loaded: %s (%s)", info->name, info->version);
    return 0;
}

static void plugin_unload_library(PluginInfo* info) {
    if (info->handle) {
        // Get plugin API for cleanup
        PluginAPI* api = (PluginAPI*)dlsym(info->handle, "plugin_api");
        if (api && api->cleanup) {
            api->cleanup();
        }

        dlclose(info->handle);
        info->handle = NULL;
        LOG_INFO("Plugin unloaded: %s", info->name);
    }
}

PluginManager* plugin_manager_create(const char* plugins_dir) {
    if (!plugins_dir) {
        LOG_ERROR("Invalid plugins directory");
        return NULL;
    }

    PluginManager* manager = (PluginManager*)malloc(sizeof(PluginManager));
    if (!manager) {
        LOG_ERROR("Failed to allocate plugin manager");
        return NULL;
    }

    manager->plugins_dir = strdup(plugins_dir);
    manager->plugins = NULL;
    manager->plugin_count = 0;
    manager->plugin_capacity = 0;

    if (!manager->plugins_dir) {
        LOG_ERROR("Failed to allocate plugins directory string");
        free(manager);
        return NULL;
    }

    LOG_INFO("Plugin manager created for directory: %s", plugins_dir);
    return manager;
}

void plugin_manager_destroy(PluginManager* manager) {
    if (!manager) return;

    for (size_t i = 0; i < manager->plugin_count; i++) {
        plugin_info_free(manager->plugins[i]);
    }

    free(manager->plugins);
    free(manager->plugins_dir);
    free(manager);

    LOG_INFO("Plugin manager destroyed");
}

int plugin_manager_load_plugins(PluginManager* manager) {
    if (!manager) return -1;

    DIR* dir = opendir(manager->plugins_dir);
    if (!dir) {
        LOG_WARNING("Failed to open plugins directory: %s", manager->plugins_dir);
        return 0; // Not an error if directory doesn't exist
    }

    struct dirent* entry;
    int loaded_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        // Check if it's a directory
        char plugin_path[PATH_MAX];
        snprintf(plugin_path, sizeof(plugin_path), "%s/%s", manager->plugins_dir, entry->d_name);

        struct stat st;
        if (stat(plugin_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            continue;
        }

        // Check for manifest file
        char manifest_path[PATH_MAX];
        size_t plugin_len = strlen(plugin_path);
        size_t manifest_len = strlen(PLUGIN_MANIFEST);

        if (plugin_len + 1 + manifest_len >= sizeof(manifest_path)) {
            LOG_WARNING("Plugin path too long, skipping: %s", plugin_path);
            continue;
        }

        memcpy(manifest_path, plugin_path, plugin_len);
        manifest_path[plugin_len] = '/';
        memcpy(manifest_path + plugin_len + 1, PLUGIN_MANIFEST, manifest_len);
        manifest_path[plugin_len + 1 + manifest_len] = '\0';

        if (access(manifest_path, F_OK) != 0) {
            continue;
        }

        // Load plugin info
        PluginInfo* info = (PluginInfo*)malloc(sizeof(PluginInfo));
        if (!info) {
            LOG_ERROR("Failed to allocate plugin info");
            continue;
        }

        memset(info, 0, sizeof(PluginInfo));
        info->path = strdup(plugin_path);

        if (plugin_info_load_from_manifest(info, manifest_path) != 0) {
            plugin_info_free(info);
            continue;
        }

        // Check if plugin should be enabled
        char config_key[256];
        snprintf(config_key, sizeof(config_key), "plugins.%s.enabled", info->id);
        info->enabled = config_get_bool(config_key, true);

        // Load plugin library if enabled
        if (info->enabled) {
            if (plugin_load_library(info) != 0) {
                LOG_WARNING("Failed to load plugin: %s", info->id);
                info->enabled = false;
            }
        }

        // Add to plugins array
        if (manager->plugin_count >= manager->plugin_capacity) {
            size_t new_capacity = manager->plugin_capacity == 0 ? 8 : manager->plugin_capacity * 2;
            PluginInfo** new_plugins = (PluginInfo**)realloc(manager->plugins, sizeof(PluginInfo*) * new_capacity);
            if (!new_plugins) {
                LOG_ERROR("Failed to resize plugins array");
                plugin_info_free(info);
                continue;
            }
            manager->plugins = new_plugins;
            manager->plugin_capacity = new_capacity;
        }

        manager->plugins[manager->plugin_count++] = info;
        loaded_count++;
    }

    closedir(dir);
    LOG_INFO("Loaded %d plugins from %s", loaded_count, manager->plugins_dir);
    return loaded_count;
}

int plugin_manager_enable_plugin(PluginManager* manager, const char* plugin_id) {
    if (!manager || !plugin_id) return -1;

    for (size_t i = 0; i < manager->plugin_count; i++) {
        PluginInfo* info = manager->plugins[i];
        if (strcmp(info->id, plugin_id) == 0) {
            if (!info->enabled) {
                if (plugin_load_library(info) == 0) {
                    info->enabled = true;
                    LOG_INFO("Plugin enabled: %s", plugin_id);
                    return 0;
                } else {
                    LOG_ERROR("Failed to enable plugin: %s", plugin_id);
                    return -1;
                }
            }
            return 0; // Already enabled
        }
    }

    LOG_ERROR("Plugin not found: %s", plugin_id);
    return -1;
}

int plugin_manager_disable_plugin(PluginManager* manager, const char* plugin_id) {
    if (!manager || !plugin_id) return -1;

    for (size_t i = 0; i < manager->plugin_count; i++) {
        PluginInfo* info = manager->plugins[i];
        if (strcmp(info->id, plugin_id) == 0) {
            if (info->enabled) {
                plugin_unload_library(info);
                info->enabled = false;
                LOG_INFO("Plugin disabled: %s", plugin_id);
            }
            return 0;
        }
    }

    LOG_ERROR("Plugin not found: %s", plugin_id);
    return -1;
}

const PluginInfo* plugin_manager_get_plugin_info(PluginManager* manager, const char* plugin_id) {
    if (!manager || !plugin_id) return NULL;

    for (size_t i = 0; i < manager->plugin_count; i++) {
        if (strcmp(manager->plugins[i]->id, plugin_id) == 0) {
            return manager->plugins[i];
        }
    }

    return NULL;
}

const PluginInfo** plugin_manager_get_plugins(PluginManager* manager, size_t* count) {
    if (!manager || !count) return NULL;

    *count = manager->plugin_count;
    return (const PluginInfo**)manager->plugins;
}

int plugin_manager_call_hook(PluginManager* manager, const char* hook_name, void* data) {
    if (!manager || !hook_name) return -1;

    int result = 0;

    for (size_t i = 0; i < manager->plugin_count; i++) {
        PluginInfo* info = manager->plugins[i];
        if (!info->enabled || !info->handle) continue;

        PluginAPI* api = (PluginAPI*)dlsym(info->handle, "plugin_api");
        if (api && api->hook) {
            int hook_result = api->hook(hook_name, data);
            if (hook_result != 0) {
                LOG_WARNING("Plugin %s hook '%s' returned error: %d", info->id, hook_name, hook_result);
                result = -1;
            }
        }
    }

    return result;
}