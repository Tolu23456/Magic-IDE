#ifndef ASYNC_IPC_H
#define ASYNC_IPC_H

#include <webkit2/webkit2.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <pthread.h>

// Async IPC message
typedef struct {
    char* id;
    char* method;
    struct json_object* params;
    void (*callback)(const char* response, void* user_data);
    void* user_data;
    struct AsyncIPCMessage* next;
} AsyncIPCMessage;

// Async IPC queue
typedef struct {
    AsyncIPCMessage* head;
    AsyncIPCMessage* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool shutdown;
    pthread_t worker_thread;
} AsyncIPCQueue;

/**
 * @brief Initialize async IPC system
 *
 * @return 0 on success, -1 on failure
 */
int async_ipc_init(void);

/**
 * @brief Shutdown async IPC system
 */
void async_ipc_shutdown(void);

/**
 * @brief Send async IPC message
 *
 * @param web_view WebView instance
 * @param method IPC method name
 * @param params JSON parameters
 * @param callback Response callback function
 * @param user_data User data for callback
 * @return Message ID, NULL on failure
 */
const char* async_ipc_send_message(WebKitWebView* web_view, const char* method,
                                   struct json_object* params,
                                   void (*callback)(const char* response, void* user_data),
                                   void* user_data);

/**
 * @brief Handle incoming IPC response
 *
 * @param message_id Message ID
 * @param response JSON response
 */
void async_ipc_handle_response(const char* message_id, const char* response);

/**
 * @brief Process pending IPC messages
 *
 * @param web_view WebView instance
 */
void async_ipc_process_queue(WebKitWebView* web_view);

#endif // ASYNC_IPC_H