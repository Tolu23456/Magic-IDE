#ifndef IPC_H
#define IPC_H

#include <json-c/json.h>
#include <webkit2/webkit2.h>

// IPC function declarations
void send_message_to_frontend(WebKitWebView* web_view, const char* message);
void handle_frontend_message(WebKitWebView* web_view, const char* message);

#endif // IPC_H