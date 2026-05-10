#include "webview.h"
#include <webkit2/webkit2.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "file_manager.h"
#include "ipc.h"

static char* build_base_uri(const char* html_path) {
    if (!html_path) {
        return strdup("file:///");
    }

    char resolved[PATH_MAX];
    const char* resolved_path = realpath(html_path, resolved);
    if (!resolved_path) {
        resolved_path = html_path;
    }

    char* directory = strdup(resolved_path);
    if (!directory) {
        return strdup("file:///");
    }

    char* last_sep = strrchr(directory, '/');
    if (last_sep) {
        *last_sep = '\0';
    }

    size_t uri_len = strlen("file://") + strlen(directory) + 2;
    char* base_uri = (char*)malloc(uri_len);
    if (base_uri) {
        snprintf(base_uri, uri_len, "file://%s/", directory);
    }

    free(directory);
    return base_uri;
}

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
            char* base_uri = build_base_uri(html_path);
            webkit_web_view_load_html(web_view, html_content, base_uri);
            free(html_content);
            free(base_uri);
            return;
        }

        fprintf(stderr, "Failed to read HTML file: %s\n", html_path);
        const char* fallback_html =
            "<!DOCTYPE html><html><body>"
            "<h1>Magic IDE</h1><p>Loading...</p>"
            "</body></html>";
        webkit_web_view_load_html(web_view, fallback_html, NULL);
        return;
    }

    fprintf(stderr, "HTML file not found: %s\n", html_path);
    const char* error_html =
        "<!DOCTYPE html><html><body>"
        "<h1>Magic IDE</h1><p>HTML file not found</p>"
        "</body></html>";
    webkit_web_view_load_html(web_view, error_html, NULL);
}

void webview_inject_ipc_script(WebKitWebView* web_view) {
    if (!web_view) {
        return;
    }

    const char* ipc_script =
        "window.magicIDE = {"
        "  callbacks: {},"
        "  sendMessage: function(method, params, callback) {"
        "    var id = 'magicide_' + Math.random().toString(36).substr(2, 9);"
        "    if (callback) { this.callbacks[id] = callback; }"
        "    var payload = { method: method, params: params, id: id };"
        "    window.location.href = 'magicide://' + encodeURIComponent(JSON.stringify(payload));"
        "  },"
        "  readFile: function(filename, callback) { this.sendMessage('readFile', { filename: filename }, callback); },"
        "  writeFile: function(filename, content, callback) { this.sendMessage('writeFile', { filename: filename, content: content }, callback); },"
        "  listDirectory: function(dirname, callback) { this.sendMessage('listDirectory', { dirname: dirname }, callback); },"
        "  handleResponse: function(response) {"
        "    if (!response) { return; }"
        "    if (response.id && this.callbacks[response.id]) {"
        "      this.callbacks[response.id](response);"
        "      delete this.callbacks[response.id];"
        "      return;"
        "    }"
        "    if (this.onMessage) { this.onMessage(response); }"
        "  }"
        "};"
        "window.addEventListener('message', function(event) {"
        "  if (event.data && typeof event.data === 'object') {"
        "    window.magicIDE.handleResponse(event.data);"
        "  }"
        "});";

    webkit_web_view_evaluate_javascript(web_view, ipc_script, -1, NULL, NULL, NULL, NULL, NULL);
}

void webview_setup_callbacks(WebKitWebView* web_view, GtkWidget* window) {
    (void)window;

    if (!web_view) {
        return;
    }

    g_signal_connect(web_view, "load-changed", G_CALLBACK(webview_load_changed_cb), NULL);
    g_signal_connect(web_view, "decide-policy", G_CALLBACK(webview_decide_policy_cb), NULL);
    g_signal_connect(web_view, "script-dialog", G_CALLBACK(webview_script_dialog_cb), NULL);
}

void webview_load_changed_cb(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    (void)user_data;

    if (load_event == WEBKIT_LOAD_FINISHED) {
        webview_inject_ipc_script(web_view);
    }
}

gboolean webview_decide_policy_cb(WebKitWebView* web_view, WebKitPolicyDecision* decision, WebKitPolicyDecisionType decision_type, gpointer user_data) {
    (void)user_data;

    if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
        WebKitNavigationPolicyDecision* nav_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        WebKitNavigationAction* action = webkit_navigation_policy_decision_get_navigation_action(nav_decision);
        WebKitURIRequest* request = webkit_navigation_action_get_request(action);
        const char* uri = webkit_uri_request_get_uri(request);

        if (g_str_has_prefix(uri, "magicide://")) {
            const char* encoded_message = uri + strlen("magicide://");
            char* decoded_message = g_uri_unescape_string(encoded_message, NULL);
            if (decoded_message) {
                handle_frontend_message(web_view, decoded_message);
                g_free(decoded_message);
            }
            webkit_policy_decision_ignore(decision);
            return TRUE;
        }
    }

    return FALSE;
}

void webview_script_dialog_cb(WebKitWebView* web_view, WebKitScriptDialog* dialog, gpointer user_data) {
    (void)web_view;
    (void)user_data;

    WebKitScriptDialogType type = webkit_script_dialog_get_dialog_type(dialog);
    if (type == WEBKIT_SCRIPT_DIALOG_ALERT || type == WEBKIT_SCRIPT_DIALOG_CONFIRM || type == WEBKIT_SCRIPT_DIALOG_PROMPT) {
        const char* message = webkit_script_dialog_get_message(dialog);
        printf("JavaScript Dialog: %s\n", message);
        webkit_script_dialog_close(dialog);
    }
}
