#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_manager.h"
#include "ipc.h"

static void destroy_window_cb(GtkWidget* widget, GtkWidget* window);
static gboolean close_web_view_cb(WebKitWebView* web_view, GtkWidget* window);
static void web_view_load_changed_cb(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data);
static gboolean web_view_decide_policy_cb(WebKitWebView* web_view, WebKitPolicyDecision* decision, WebKitPolicyDecisionType decision_type, gpointer user_data);
static void web_view_script_dialog_cb(WebKitWebView* web_view, WebKitScriptDialog* dialog, gpointer user_data);

static WebKitWebView* main_web_view = NULL;

int main(int argc, char* argv[]) {
    GtkWidget *main_window;
    GtkWidget *web_view;

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create main window
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Magic IDE");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 800);
    g_signal_connect(main_window, "destroy", G_CALLBACK(destroy_window_cb), NULL);

    // Create WebKit WebView
    web_view = webkit_web_view_new();
    main_web_view = WEBKIT_WEB_VIEW(web_view);
    g_signal_connect(web_view, "close", G_CALLBACK(close_web_view_cb), main_window);
    g_signal_connect(web_view, "load-changed", G_CALLBACK(web_view_load_changed_cb), NULL);
    g_signal_connect(web_view, "decide-policy", G_CALLBACK(web_view_decide_policy_cb), NULL);
    g_signal_connect(web_view, "script-dialog", G_CALLBACK(web_view_script_dialog_cb), NULL);

    // Load initial HTML
    char* html_path = "web/index.html";
    if (file_exists(html_path)) {
        char* html_content = read_file(html_path);
        if (html_content) {
            webkit_web_view_load_html(WEBKIT_WEB_VIEW(web_view), html_content, "file:///");
            free(html_content);
        } else {
            // Fallback HTML
            webkit_web_view_load_html(WEBKIT_WEB_VIEW(web_view),
                "<!DOCTYPE html><html><body><h1>Magic IDE</h1><p>Loading...</p></body></html>", NULL);
        }
    } else {
        // Fallback HTML
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(web_view),
            "<!DOCTYPE html><html><body><h1>Magic IDE</h1><p>HTML file not found</p></body></html>", NULL);
    }

    // Add web view to window
    gtk_container_add(GTK_CONTAINER(main_window), web_view);
    gtk_widget_show_all(main_window);

    // Run GTK main loop
    gtk_main();

    return 0;
}

static void destroy_window_cb(GtkWidget* widget, GtkWidget* window) {
    gtk_main_quit();
}

static gboolean close_web_view_cb(WebKitWebView* web_view, GtkWidget* window) {
    gtk_widget_destroy(window);
    return TRUE;
}

static void web_view_load_changed_cb(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    if (load_event == WEBKIT_LOAD_FINISHED) {
        // Inject IPC script
        const char* ipc_script =
            "window.magicIDE = {"
            "  sendMessage: function(method, params) {"
            "    window.postMessage({method: method, params: params}, '*');"
            "  },"
            "  readFile: function(filename, callback) {"
            "    this.sendMessage('readFile', {filename: filename});"
            "  },"
            "  writeFile: function(filename, content, callback) {"
            "    this.sendMessage('writeFile', {filename: filename, content: content});"
            "  },"
            "  listDirectory: function(dirname, callback) {"
            "    this.sendMessage('listDirectory', {dirname: dirname});"
            "  }"
            "};"
            ""
            "window.addEventListener('message', function(event) {"
            "  if (event.data && typeof event.data === 'object') {"
            "    // Handle messages from backend"
            "    console.log('Received message from backend:', event.data);"
            "  }"
            "});";

        webkit_web_view_run_javascript(web_view, ipc_script, NULL, NULL, NULL);
    }
}

static gboolean web_view_decide_policy_cb(WebKitWebView* web_view, WebKitPolicyDecision* decision, WebKitPolicyDecisionType decision_type, gpointer user_data) {
    if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
        WebKitNavigationAction* action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
        WebKitURIRequest* request = webkit_navigation_action_get_request(action);
        const char* uri = webkit_uri_request_get_uri(request);

        // Handle custom URI scheme for IPC
        if (g_str_has_prefix(uri, "magicide://")) {
            // Extract message from URI
            const char* message = uri + strlen("magicide://");
            handle_frontend_message(web_view, message);
            webkit_policy_decision_ignore(decision);
            return TRUE;
        }
    }
    return FALSE;
}

static void web_view_script_dialog_cb(WebKitWebView* web_view, WebKitScriptDialog* dialog, gpointer user_data) {
    WebKitScriptDialogType type = webkit_script_dialog_get_dialog_type(dialog);
    if (type == WEBKIT_SCRIPT_DIALOG_ALERT || type == WEBKIT_SCRIPT_DIALOG_CONFIRM || type == WEBKIT_SCRIPT_DIALOG_PROMPT) {
        const char* message = webkit_script_dialog_get_message(dialog);
        printf("JavaScript Dialog: %s\n", message);
    }
}