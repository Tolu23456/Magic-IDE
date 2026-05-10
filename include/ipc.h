#ifndef IPC_H
#define IPC_H

#include <json-c/json.h>
#include <webkit2/webkit2.h>

/**
 * @brief Inter-Process Communication (IPC) module for Magic IDE
 *
 * This module handles communication between the C backend and JavaScript frontend
 * using JSON-RPC 2.0 over WebKit's postMessage API.
 */

/**
 * @brief Send a message to the frontend JavaScript code
 *
 * @param web_view The WebKit WebView instance
 * @param message JSON string to send to the frontend
 */
void send_message_to_frontend(WebKitWebView* web_view, const char* message);

/**
 * @brief Handle a message received from the frontend JavaScript code
 *
 * @param web_view The WebKit WebView instance
 * @param message JSON string received from the frontend
 */
void handle_frontend_message(WebKitWebView* web_view, const char* message);

#endif // IPC_H