#include "async_ipc.h"
#include "logger.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Global async IPC queue
static AsyncIPCQueue* g_async_queue = NULL;

// Message ID generator
static unsigned int g_message_id_counter = 0;
static pthread_mutex_t g_message_id_mutex = PTHREAD_MUTEX_INITIALIZER;

static char* generate_message_id(void) {
    pthread_mutex_lock(&g_message_id_mutex);

    time_t now = time(NULL);
    unsigned int counter = g_message_id_counter++;

    char* id = (char*)malloc(32);
    if (id) {
        snprintf(id, 32, "msg_%lx_%x", (unsigned long)now, counter);
    }

    pthread_mutex_unlock(&g_message_id_mutex);
    return id;
}

static void* async_ipc_worker_thread(void* arg) {
    (void)arg; // Unused

    LOG_DEBUG("Async IPC worker thread started");

    while (true) {
        pthread_mutex_lock(&g_async_queue->mutex);

        while (!g_async_queue->shutdown && !g_async_queue->head) {
            pthread_cond_wait(&g_async_queue->cond, &g_async_queue->mutex);
        }

        if (g_async_queue->shutdown) {
            pthread_mutex_unlock(&g_async_queue->mutex);
            break;
        }

        // Get next message
        AsyncIPCMessage* message = g_async_queue->head;
        if (message) {
            g_async_queue->head = message->next;
            if (!g_async_queue->head) {
                g_async_queue->tail = NULL;
            }
        }

        pthread_mutex_unlock(&g_async_queue->mutex);

        if (message) {
            // Process message (this would be called from the main thread)
            LOG_DEBUG("Processing async IPC message: %s", message->id);

            // In a real implementation, this would trigger the actual IPC call
            // For now, we'll just simulate processing

            // Free message
            free(message->id);
            free(message->method);
            if (message->params) {
                json_object_put(message->params);
            }
            free(message);
        }
    }

    LOG_DEBUG("Async IPC worker thread stopped");
    return NULL;
}

int async_ipc_init(void) {
    if (g_async_queue) {
        LOG_WARNING("Async IPC already initialized");
        return 0;
    }

    g_async_queue = (AsyncIPCQueue*)malloc(sizeof(AsyncIPCQueue));
    if (!g_async_queue) {
        LOG_ERROR("Failed to allocate async IPC queue");
        return -1;
    }

    g_async_queue->head = NULL;
    g_async_queue->tail = NULL;
    g_async_queue->shutdown = false;

    if (pthread_mutex_init(&g_async_queue->mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize async IPC mutex");
        free(g_async_queue);
        g_async_queue = NULL;
        return -1;
    }

    if (pthread_cond_init(&g_async_queue->cond, NULL) != 0) {
        LOG_ERROR("Failed to initialize async IPC condition variable");
        pthread_mutex_destroy(&g_async_queue->mutex);
        free(g_async_queue);
        g_async_queue = NULL;
        return -1;
    }

    if (pthread_create(&g_async_queue->worker_thread, NULL, async_ipc_worker_thread, NULL) != 0) {
        LOG_ERROR("Failed to create async IPC worker thread");
        pthread_cond_destroy(&g_async_queue->cond);
        pthread_mutex_destroy(&g_async_queue->mutex);
        free(g_async_queue);
        g_async_queue = NULL;
        return -1;
    }

    LOG_INFO("Async IPC system initialized");
    return 0;
}

void async_ipc_shutdown(void) {
    if (!g_async_queue) return;

    pthread_mutex_lock(&g_async_queue->mutex);
    g_async_queue->shutdown = true;
    pthread_cond_signal(&g_async_queue->cond);
    pthread_mutex_unlock(&g_async_queue->mutex);

    pthread_join(g_async_queue->worker_thread, NULL);

    // Free remaining messages
    AsyncIPCMessage* current = g_async_queue->head;
    while (current) {
        AsyncIPCMessage* next = current->next;
        free(current->id);
        free(current->method);
        if (current->params) {
            json_object_put(current->params);
        }
        free(current);
        current = next;
    }

    pthread_cond_destroy(&g_async_queue->cond);
    pthread_mutex_destroy(&g_async_queue->mutex);
    free(g_async_queue);
    g_async_queue = NULL;

    LOG_INFO("Async IPC system shutdown");
}

const char* async_ipc_send_message(WebKitWebView* web_view, const char* method,
                                   struct json_object* params,
                                   void (*callback)(const char* response, void* user_data),
                                   void* user_data) {
    if (!g_async_queue || !web_view || !method) {
        return NULL;
    }

    AsyncIPCMessage* message = (AsyncIPCMessage*)malloc(sizeof(AsyncIPCMessage));
    if (!message) {
        LOG_ERROR("Failed to allocate async IPC message");
        return NULL;
    }

    message->id = generate_message_id();
    message->method = strdup(method);
    message->params = params ? json_object_get(params) : NULL; // Increase ref count
    message->callback = callback;
    message->user_data = user_data;
    message->next = NULL;

    if (!message->id || !message->method) {
        LOG_ERROR("Failed to allocate message components");
        if (message->params) json_object_put(message->params);
        free(message->method);
        free(message->id);
        free(message);
        return NULL;
    }

    // Add to queue
    pthread_mutex_lock(&g_async_queue->mutex);

    if (g_async_queue->tail) {
        g_async_queue->tail->next = message;
    } else {
        g_async_queue->head = message;
    }
    g_async_queue->tail = message;

    pthread_cond_signal(&g_async_queue->cond);
    pthread_mutex_unlock(&g_async_queue->mutex);

    LOG_DEBUG("Queued async IPC message: %s (%s)", message->id, method);

    // Immediately send the message (in a real implementation, this would be done by the worker)
    char* ipc_message = NULL;
    size_t ipc_size = strlen("{\"jsonrpc\":\"2.0\",\"id\":\"\",\"method\":\"\"}") +
                      strlen(message->id) + strlen(method) + 1;

    if (message->params) {
        const char* params_str = json_object_to_json_string(message->params);
        ipc_size += strlen(",\"params\":") + strlen(params_str);
    }

    ipc_message = (char*)malloc(ipc_size);
    if (ipc_message) {
        if (message->params) {
            const char* params_str = json_object_to_json_string(message->params);
            snprintf(ipc_message, ipc_size, "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"method\":\"%s\",\"params\":%s}",
                     message->id, method, params_str);
        } else {
            snprintf(ipc_message, ipc_size, "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"method\":\"%s\"}",
                     message->id, method);
        }

        send_message_to_frontend(web_view, ipc_message);
        free(ipc_message);
    }

    return message->id;
}

void async_ipc_handle_response(const char* message_id, const char* response) {
    if (!g_async_queue || !message_id) return;

    // In a real implementation, this would find the message by ID and call its callback
    LOG_DEBUG("Handling async IPC response for message: %s", message_id);

    // For now, just log the response
    if (response) {
        LOG_DEBUG("Response: %s", response);
    }
}

void async_ipc_process_queue(WebKitWebView* web_view) {
    (void)web_view; // Unused for now

    // This function would be called periodically to process queued messages
    // In the current implementation, messages are sent immediately
}