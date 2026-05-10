#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <json-c/json.h>
#include <stdbool.h>

// Plugin information
typedef struct {
    char* id;
    char* name;
    char* version;
    char* description;
    char* author;
    char** dependencies;
    size_t dependency_count;
    bool enabled;
    void* handle; // Dynamic library handle
    char* path;
} PluginInfo;

// Plugin manager
typedef struct PluginManager PluginManager;

/**
 * @brief Initialize plugin manager
 *
 * @param plugins_dir Directory containing plugins
 * @return Plugin manager instance, NULL on failure
 */
PluginManager* plugin_manager_create(const char* plugins_dir);

/**
 * @brief Destroy plugin manager
 *
 * @param manager Plugin manager to destroy
 */
void plugin_manager_destroy(PluginManager* manager);

/**
 * @brief Load all plugins from plugins directory
 *
 * @param manager Plugin manager
 * @return Number of plugins loaded, -1 on error
 */
int plugin_manager_load_plugins(PluginManager* manager);

/**
 * @brief Enable a plugin
 *
 * @param manager Plugin manager
 * @param plugin_id Plugin ID to enable
 * @return 0 on success, -1 on failure
 */
int plugin_manager_enable_plugin(PluginManager* manager, const char* plugin_id);

/**
 * @brief Disable a plugin
 *
 * @param manager Plugin manager
 * @param plugin_id Plugin ID to disable
 * @return 0 on success, -1 on failure
 */
int plugin_manager_disable_plugin(PluginManager* manager, const char* plugin_id);

/**
 * @brief Get plugin information
 *
 * @param manager Plugin manager
 * @param plugin_id Plugin ID
 * @return Plugin info, NULL if not found
 */
const PluginInfo* plugin_manager_get_plugin_info(PluginManager* manager, const char* plugin_id);

/**
 * @brief Get list of all plugins
 *
 * @param manager Plugin manager
 * @param count Output parameter for number of plugins
 * @return Array of plugin info pointers, NULL on failure
 */
const PluginInfo** plugin_manager_get_plugins(PluginManager* manager, size_t* count);

/**
 * @brief Call a hook function across all enabled plugins
 *
 * @param manager Plugin manager
 * @param hook_name Name of the hook to call
 * @param data Hook-specific data
 * @return 0 on success, -1 on failure
 */
int plugin_manager_call_hook(PluginManager* manager, const char* hook_name, void* data);

// Plugin API for plugins to implement

/**
 * @brief Plugin initialization function signature
 */
typedef int (*PluginInitFunc)(void);

/**
 * @brief Plugin cleanup function signature
 */
typedef void (*PluginCleanupFunc)(void);

/**
 * @brief Plugin hook function signature
 */
typedef int (*PluginHookFunc)(const char* hook_name, void* data);

/**
 * @brief Plugin API structure that plugins must export
 */
typedef struct {
    const char* api_version;
    PluginInitFunc init;
    PluginCleanupFunc cleanup;
    PluginHookFunc hook;
} PluginAPI;

#endif // PLUGIN_MANAGER_H