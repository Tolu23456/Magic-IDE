#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit2.h>
#include "file_manager.h"

// Forward declarations for helper functions
static void handle_read_file_request(WebKitWebView* web_view, struct json_object* request);
static void handle_write_file_request(WebKitWebView* web_view, struct json_object* request);
static void handle_list_directory_request(WebKitWebView* web_view, struct json_object* request);
static void send_error_response(WebKitWebView* web_view, const char* error_message);
static char* escape_json_string(const char* str);

// IPC functions for communication between C and JavaScript

void send_message_to_frontend(WebKitWebView* web_view, const char* message) {
    if (!web_view || !message) {
        return;
    }

    // Calculate required buffer size safely
    size_t script_size = strlen("window.postMessage(, '*');") + strlen(message) + 1;
    char* script = (char*)malloc(script_size);
    if (!script) {
        fprintf(stderr, "Failed to allocate memory for IPC script\n");
        return;
    }

    // Use snprintf for safe string formatting
    int written = snprintf(script, script_size, "window.postMessage(%s, '*');", message);
    if (written < 0 || (size_t)written >= script_size) {
        fprintf(stderr, "Failed to format IPC script\n");
        free(script);
        return;
    }

    webkit_web_view_evaluate_javascript(web_view, script, -1, NULL, NULL, NULL, NULL, NULL);
    free(script);
}

void handle_frontend_message(WebKitWebView* web_view, const char* message) {
    if (!web_view || !message) {
        return;
    }

    // Parse JSON message from frontend
    struct json_object* parsed_json = json_tokener_parse(message);
    if (!parsed_json) {
        fprintf(stderr, "Failed to parse JSON message: %s\n", message);
        return;
    }

    struct json_object* method_obj = NULL;
    if (!json_object_object_get_ex(parsed_json, "method", &method_obj)) {
        fprintf(stderr, "No method field in JSON message\n");
        json_object_put(parsed_json);
        return;
    }

    const char* method = json_object_get_string(method_obj);
    if (!method) {
        fprintf(stderr, "Method field is not a string\n");
        json_object_put(parsed_json);
        return;
    }

    if (strcmp(method, "readFile") == 0) {
        handle_read_file_request(web_view, parsed_json);
    } else if (strcmp(method, "writeFile") == 0) {
        handle_write_file_request(web_view, parsed_json);
    } else if (strcmp(method, "listDirectory") == 0) {
        handle_list_directory_request(web_view, parsed_json);
    } else {
        fprintf(stderr, "Unknown method: %s\n", method);
        send_error_response(web_view, "Unknown method");
    }

    json_object_put(parsed_json);
}

// Helper functions for handling different IPC requests

static void handle_read_file_request(WebKitWebView* web_view, struct json_object* request) {
    struct json_object* params_obj = NULL;
    if (!json_object_object_get_ex(request, "params", &params_obj)) {
        send_error_response(web_view, "Missing params in readFile request");
        return;
    }

    struct json_object* filename_obj = NULL;
    if (!json_object_object_get_ex(params_obj, "filename", &filename_obj)) {
        send_error_response(web_view, "Missing filename in readFile request");
        return;
    }

    const char* filename = json_object_get_string(filename_obj);
    if (!filename) {
        send_error_response(web_view, "Invalid filename in readFile request");
        return;
    }

    // Read file and send response
    char* content = read_file(filename);
    if (content) {
        // Escape quotes and backslashes for JSON safety
        char* escaped_content = escape_json_string(content);
        if (escaped_content) {
            char* response = NULL;
            size_t response_size = strlen("{\"result\": \"\"}") + strlen(escaped_content) + 1;
            response = (char*)malloc(response_size);
            if (response) {
                snprintf(response, response_size, "{\"result\": \"%s\"}", escaped_content);
                send_message_to_frontend(web_view, response);
                free(response);
            }
            free(escaped_content);
        }
        free(content);
    } else {
        send_error_response(web_view, "Failed to read file");
    }
}

static void handle_write_file_request(WebKitWebView* web_view, struct json_object* request) {
    struct json_object* params_obj = NULL;
    if (!json_object_object_get_ex(request, "params", &params_obj)) {
        send_error_response(web_view, "Missing params in writeFile request");
        return;
    }

    struct json_object* filename_obj = NULL;
    struct json_object* content_obj = NULL;
    if (!json_object_object_get_ex(params_obj, "filename", &filename_obj) ||
        !json_object_object_get_ex(params_obj, "content", &content_obj)) {
        send_error_response(web_view, "Missing filename or content in writeFile request");
        return;
    }

    const char* filename = json_object_get_string(filename_obj);
    const char* content = json_object_get_string(content_obj);
    if (!filename || !content) {
        send_error_response(web_view, "Invalid filename or content in writeFile request");
        return;
    }

    int result = write_file(filename, content);
    const char* response = result == 0 ? "{\"result\": true}" : "{\"result\": false}";
    send_message_to_frontend(web_view, response);
}

static void handle_list_directory_request(WebKitWebView* web_view, struct json_object* request) {
    struct json_object* params_obj = NULL;
    if (!json_object_object_get_ex(request, "params", &params_obj)) {
        send_error_response(web_view, "Missing params in listDirectory request");
        return;
    }

    struct json_object* dirname_obj = NULL;
    if (!json_object_object_get_ex(params_obj, "dirname", &dirname_obj)) {
        send_error_response(web_view, "Missing dirname in listDirectory request");
        return;
    }

    const char* dirname = json_object_get_string(dirname_obj);
    if (!dirname) {
        send_error_response(web_view, "Invalid dirname in listDirectory request");
        return;
    }

    int count = 0;
    char** files = list_directory(dirname, &count);
    if (!files) {
        send_error_response(web_view, "Failed to list directory");
        return;
    }

    // Create JSON response
    struct json_object* response_obj = json_object_new_object();
    struct json_object* files_array = json_object_new_array();

    for (int i = 0; i < count; i++) {
        json_object_array_add(files_array, json_object_new_string(files[i]));
    }

    json_object_object_add(response_obj, "files", files_array);
    const char* response_str = json_object_to_json_string(response_obj);

    send_message_to_frontend(web_view, response_str);

    json_object_put(response_obj);
    free_file_list(files, count);
}

static void send_error_response(WebKitWebView* web_view, const char* error_message) {
    char* response = NULL;
    size_t response_size = strlen("{\"error\": \"\"}") + strlen(error_message) + 1;
    response = (char*)malloc(response_size);
    if (response) {
        snprintf(response, response_size, "{\"error\": \"%s\"}", error_message);
        send_message_to_frontend(web_view, response);
        free(response);
    }
}

static char* escape_json_string(const char* str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    // Worst case: every character needs escaping (len * 2 + 1 for null terminator)
    char* escaped = (char*)malloc(len * 2 + 1);
    if (!escaped) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        switch (c) {
            case '"':
                escaped[j++] = '\\';
                escaped[j++] = '"';
                break;
            case '\\':
                escaped[j++] = '\\';
                escaped[j++] = '\\';
                break;
            case '\n':
                escaped[j++] = '\\';
                escaped[j++] = 'n';
                break;
            case '\r':
                escaped[j++] = '\\';
                escaped[j++] = 'r';
                break;
            case '\t':
                escaped[j++] = '\\';
                escaped[j++] = 't';
                break;
            default:
                escaped[j++] = c;
                break;
        }
    }
    escaped[j] = '\0';
    return escaped;
}