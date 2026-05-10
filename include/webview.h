#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

// WebView management functions
WebKitWebView* webview_create(void);
void webview_load_html(WebKitWebView* web_view, const char* html_path);
void webview_inject_ipc_script(WebKitWebView* web_view);
void webview_setup_callbacks(WebKitWebView* web_view, GtkWidget* window);

// Callback functions
void webview_load_changed_cb(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data);
gboolean webview_decide_policy_cb(WebKitWebView* web_view, WebKitPolicyDecision* decision, WebKitPolicyDecisionType decision_type, gpointer user_data);
void webview_script_dialog_cb(WebKitWebView* web_view, WebKitScriptDialog* dialog, gpointer user_data);

#endif // WEBVIEW_H