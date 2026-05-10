#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <webkit2/webkit2.h>
#include "file_manager.h"

// Forward declarations for helper functions
static void handle_read_file_request(WebKitWebView* web_view, struct json_object* request);
static void handle_write_file_request(WebKitWebView* web_view, struct json_object* request);
static void handle_list_directory_request(WebKitWebView* web_view, struct json_object* request);
static void send_json_response(WebKitWebView* web_view, struct json_object* response_obj);
static void send_error_response(WebKitWebView* web_view, struct json_object* request, const char* error_message);
static void attach_request_id(struct json_object* request, struct json_object* response_obj);

void send_message_to_frontend(WebKitWebView* web_view, const char* message) {
    if (!web_view || !message) {
        return;
    }

    size_t script_size = strlen("window.postMessage(, '*');") + strlen(message) + 1;
    char* script = (char*)malloc(script_size);
    if (!script) {
        fprintf(stderr, "Failed to allocate memory for IPC script\n");
        return;
    }

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
        send_error_response(web_view, parsed_json, "Unknown method");
    }

    json_object_put(parsed_json);
}

static void handle_read_file_request(WebKitWebView* web_view, struct json_object* request) {
    struct json_object* params_obj = NULL;
    if (!json_object_object_get_ex(request, "params", &params_obj)) {
        send_error_response(web_view, request, "Missing params in readFile request");
        return;
    }

    struct json_object* filename_obj = NULL;
    if (!json_object_object_get_ex(params_obj, "filename", &filename_obj)) {
        send_error_response(web_view, request, "Missing filename in readFile request");
        return;
    }

    const char* filename = json_object_get_string(filename_obj);
    if (!filename) {
        send_error_response(web_view, request, "Invalid filename in readFile request");
        return;
    }

    char* content = read_file(filename);
    struct json_object* response_obj = json_object_new_object();
    if (content) {
        json_object_object_add(response_obj, "result", json_object_new_string(content));
        free(content);
    } else {
        json_object_object_add(response_obj, "error", json_object_new_string("Failed to read file"));
    }

    attach_request_id(request, response_obj);
    send_json_response(web_view, response_obj);
    json_object_put(response_obj);
}

static void handle_write_file_request(WebKitWebView* web_view, struct json_object* request) {
    struct json_object* params_obj = NULL;
    if (!json_object_object_get_ex(request, "params", &params_obj)) {
        send_error_response(web_view, request, "Missing params in writeFile request");
        return;
    }

    struct json_object* filename_obj = NULL;
    struct json_object* content_obj = NULL;
    if (!json_object_object_get_ex(params_obj, "filename", &filename_obj) ||
        !json_object_object_get_ex(params_obj, "content", &content_obj)) {
        send_error_response(web_view, request, "Missing filename or content in writeFile request");
        return;
    }

    const char* filename = json_object_get_string(filename_obj);
    const char* content = json_object_get_string(content_obj);
    if (!filename || !content) {
        send_error_response(web_view, request, "Invalid filename or content in writeFile request");
        return;
    }

    int result = write_file(filename, content);
    struct json_object* response_obj = json_object_new_object();
    json_object_object_add(response_obj, "result", json_object_new_boolean(result == 0));
    attach_request_id(request, response_obj);
    send_json_response(web_view, response_obj);
    json_object_put(response_obj);
}

static void handle_list_directory_request(WebKitWebView* web_view, struct json_object* request) {
    struct json_object* params_obj = NULL;
    if (!json_object_object_get_ex(request, "params", &params_obj)) {
        send_error_response(web_view, request, "Missing params in listDirectory request");
        return;
    }

    struct json_object* dirname_obj = NULL;
    if (!json_object_object_get_ex(params_obj, "dirname", &dirname_obj)) {
        send_error_response(web_view, request, "Missing dirname in listDirectory request");
        return;
    }

    const char* dirname = json_object_get_string(dirname_obj);
    if (!dirname) {
        send_error_response(web_view, request, "Invalid dirname in listDirectory request");
        return;
    }

    int count = 0;
    char** files = list_directory(dirname, &count);
    if (!files && count != 0) {
        send_error_response(web_view, request, "Failed to list directory");
        return;
    }

    struct json_object* response_obj = json_object_new_object();
    struct json_object* files_array = json_object_new_array();
    for (int i = 0; i < count; i++) {
        struct json_object* file_entry = json_object_new_object();
        json_object_object_add(file_entry, "name", json_object_new_string(files[i]));

        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s/%s", dirname, files[i]);
        json_object_object_add(file_entry, "isDirectory", json_object_new_boolean(is_directory(path)));

        json_object_array_add(files_array, file_entry);
    }

    json_object_object_add(response_obj, "result", files_array);
    attach_request_id(request, response_obj);
    send_json_response(web_view, response_obj);

    if (files) {
        free_file_list(files, count);
    }
    json_object_put(response_obj);
}

static void send_json_response(WebKitWebView* web_view, struct json_object* response_obj) {
    if (!response_obj) {
        return;
    }

    const char* response_str = json_object_to_json_string(response_obj);
    if (!response_str) {
        return;
    }
    send_message_to_frontend(web_view, response_str);
}

static void send_error_response(WebKitWebView* web_view, struct json_object* request, const char* error_message) {
    struct json_object* response_obj = json_object_new_object();
    json_object_object_add(response_obj, "error", json_object_new_string(error_message));
    attach_request_id(request, response_obj);
    send_json_response(web_view, response_obj);
    json_object_put(response_obj);
}

static void attach_request_id(struct json_object* request, struct json_object* response_obj) {
    if (!request || !response_obj) {
        return;
    }

    struct json_object* id_obj = NULL;
    if (json_object_object_get_ex(request, "id", &id_obj)) {
        json_object_object_add(response_obj, "id", json_object_get(id_obj));
    }
}
