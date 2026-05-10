#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_manager.h"
#include "ipc.h"
#include "webview.h"
#include "logger.h"
#include "config.h"
#include "memory_pool.h"
#include "plugin_manager.h"
#include "async_ipc.h"

static void destroy_window_cb(GtkWidget* widget, GtkWidget* window);

// Global instances
static PluginManager* g_plugin_manager = NULL;
static MemoryPool* g_memory_pool = NULL;
static StringPool* g_string_pool = NULL;

int main(int argc, char* argv[]) {
    // Initialize logging first
    if (logger_init(LOG_LEVEL_INFO, "~/.magic-ide/magic-ide.log", true) != 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return 1;
    }

    LOG_INFO("Starting Magic IDE");

    // Initialize configuration
    if (config_init() != 0) {
        LOG_CRITICAL("Failed to initialize configuration");
        logger_shutdown();
        return 1;
    }

    if (config_load("~/.magic-ide/config.json") != 0) {
        LOG_WARNING("Failed to load configuration, using defaults");
    }

    // Initialize memory pools
    g_memory_pool = memory_pool_create(1024, 10); // 1KB blocks, 10 initial
    g_string_pool = string_pool_create(100); // 100 initial strings

    if (!g_memory_pool || !g_string_pool) {
        LOG_CRITICAL("Failed to initialize memory pools");
        config_cleanup();
        logger_shutdown();
        return 1;
    }

    // Initialize async IPC
    if (async_ipc_init() != 0) {
        LOG_CRITICAL("Failed to initialize async IPC");
        memory_pool_destroy(g_memory_pool);
        string_pool_destroy(g_string_pool);
        config_cleanup();
        logger_shutdown();
        return 1;
    }

    // Initialize plugin manager
    g_plugin_manager = plugin_manager_create("~/.magic-ide/plugins");
    if (g_plugin_manager) {
        if (plugin_manager_load_plugins(g_plugin_manager) < 0) {
            LOG_WARNING("Failed to load plugins");
        }
    }

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create main window
    GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Magic IDE");

    // Get window size from config
    Config* config = config_get();
    int width = config_get_int("ui.window_width", 1200);
    int height = config_get_int("ui.window_height", 800);
    bool maximized = config_get_bool("ui.window_maximized", false);

    gtk_window_set_default_size(GTK_WINDOW(main_window), width, height);
    if (maximized) {
        gtk_window_maximize(GTK_WINDOW(main_window));
    }

    g_signal_connect(main_window, "destroy", G_CALLBACK(destroy_window_cb), NULL);

    // Create WebKit WebView using our webview module
    WebKitWebView* web_view = webview_create();
    if (!web_view) {
        LOG_CRITICAL("Failed to create WebView");
        plugin_manager_destroy(g_plugin_manager);
        async_ipc_shutdown();
        memory_pool_destroy(g_memory_pool);
        string_pool_destroy(g_string_pool);
        config_cleanup();
        logger_shutdown();
        return 1;
    }

    // Setup WebView callbacks
    webview_setup_callbacks(web_view, main_window);

    // Load initial HTML
    webview_load_html(web_view, "web/index.html");

    // Add web view to window
    gtk_container_add(GTK_CONTAINER(main_window), GTK_WIDGET(web_view));
    gtk_widget_show_all(main_window);

    // Call plugin hook for application startup
    if (g_plugin_manager) {
        plugin_manager_call_hook(g_plugin_manager, "app_startup", NULL);
    }

    LOG_INFO("Magic IDE started successfully");

    // Run GTK main loop
    gtk_main();

    // Cleanup
    if (g_plugin_manager) {
        plugin_manager_call_hook(g_plugin_manager, "app_shutdown", NULL);
        plugin_manager_destroy(g_plugin_manager);
    }

    async_ipc_shutdown();
    memory_pool_destroy(g_memory_pool);
    string_pool_destroy(g_string_pool);
    config_cleanup();
    logger_shutdown();

    return 0;
}

static void destroy_window_cb(GtkWidget* widget, GtkWidget* window) {
    (void)widget; // Suppress unused parameter warning
    (void)window; // Suppress unused parameter warning

    LOG_INFO("Application window destroyed, shutting down");
    gtk_main_quit();
}