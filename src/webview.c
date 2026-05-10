#include "webview.h"
#include <webkit2/webkit2.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_manager.h"
#include "ipc.h"

WebKitWebView* webview_create(void) {
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    if (!web_view) {
        fprintf(stderr, "Failed to create WebKit WebView\n");
        return NULL;
    }
    return web_view;
}

void webview_load_html(WebKitWebView* web_view, const char* html_path) {
    if (!web_view || !html_path) {
        return;
    }

    if (file_exists(html_path)) {
        char* html_content = read_file(html_path);
        if (html_content) {
            webkit_web_view_load_html(web_view, html_content, "file:///");
            free(html_content);
        } else {
            fprintf(stderr, "Failed to read HTML file: %s\n", html_path);
            // Fallback HTML
            const char* fallback_html =
                "<!DOCTYPE html><html><body>"
                "<h1>Magic IDE</h1><p>Loading...</p>"
                "</body></html>";
            webkit_web_view_load_html(web_view, fallback_html, NULL);
        }
    } else {
        fprintf(stderr, "HTML file not found: %s\n", html_path);
        const char* error_html =
            "<!DOCTYPE html><html><body>"
            "<h1>Magic IDE</h1><p>HTML file not found</p>"
            "</body></html>";
        webkit_web_view_load_html(web_view, error_html, NULL);
    }
}

void webview_inject_ipc_script(WebKitWebView* web_view) {
    if (!web_view) {
        return;
    }

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
        "    console.log('Received message from backend:', event.data);"
        "  }"
        "});";

    webkit_web_view_evaluate_javascript(web_view, ipc_script, -1, NULL, NULL, NULL, NULL, NULL);
}

void webview_setup_callbacks(WebKitWebView* web_view, GtkWidget* window) {
    (void)window; // Suppress unused parameter warning

    if (!web_view) {
        return;
    }

    g_signal_connect(web_view, "load-changed", G_CALLBACK(webview_load_changed_cb), NULL);
    g_signal_connect(web_view, "decide-policy", G_CALLBACK(webview_decide_policy_cb), NULL);
    g_signal_connect(web_view, "script-dialog", G_CALLBACK(webview_script_dialog_cb), NULL);
}

void webview_load_changed_cb(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    (void)user_data; // Suppress unused parameter warning

    if (load_event == WEBKIT_LOAD_FINISHED) {
        webview_inject_ipc_script(web_view);
    }
}

gboolean webview_decide_policy_cb(WebKitWebView* web_view, WebKitPolicyDecision* decision, WebKitPolicyDecisionType decision_type, gpointer user_data) {
    (void)user_data; // Suppress unused parameter warning

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

void webview_script_dialog_cb(WebKitWebView* web_view, WebKitScriptDialog* dialog, gpointer user_data) {
    (void)web_view; // Suppress unused parameter warning
    (void)user_data; // Suppress unused parameter warning

    WebKitScriptDialogType type = webkit_script_dialog_get_dialog_type(dialog);
    if (type == WEBKIT_SCRIPT_DIALOG_ALERT || type == WEBKIT_SCRIPT_DIALOG_CONFIRM || type == WEBKIT_SCRIPT_DIALOG_PROMPT) {
        const char* message = webkit_script_dialog_get_message(dialog);
        printf("JavaScript Dialog: %s\n", message);

        // Just close the dialog for now
        webkit_script_dialog_close(dialog);
    }
}