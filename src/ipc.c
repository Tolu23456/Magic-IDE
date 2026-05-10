#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit2.h>

// IPC functions for communication between C and JavaScript

void send_message_to_frontend(WebKitWebView* web_view, const char* message) {
    char* script = (char*)malloc(strlen(message) + 50);
    sprintf(script, "window.postMessage(%s, '*');", message);
    webkit_web_view_evaluate_javascript(web_view, script, -1, NULL, NULL, NULL, NULL, NULL);
    free(script);
}

void handle_frontend_message(WebKitWebView* web_view, const char* message) {
    // Parse JSON message from frontend
    struct json_object* parsed_json = json_tokener_parse(message);
    if (!parsed_json) {
        return;
    }

    struct json_object* method_obj;
    if (json_object_object_get_ex(parsed_json, "method", &method_obj)) {
        const char* method = json_object_get_string(method_obj);

        if (strcmp(method, "readFile") == 0) {
            // Handle file read request
            struct json_object* params_obj;
            if (json_object_object_get_ex(parsed_json, "params", &params_obj)) {
                struct json_object* filename_obj;
                if (json_object_object_get_ex(params_obj, "filename", &filename_obj)) {
                    const char* filename = json_object_get_string(filename_obj);
                    // Read file and send response
                    char* content = read_file(filename);
                    if (content) {
                        char response[1024];
                        sprintf(response, "{\"result\": \"%s\"}", content);
                        send_message_to_frontend(web_view, response);
                        free(content);
                    } else {
                        send_message_to_frontend(web_view, "{\"error\": \"Failed to read file\"}");
                    }
                }
            }
        } else if (strcmp(method, "writeFile") == 0) {
            // Handle file write request
            struct json_object* params_obj;
            if (json_object_object_get_ex(parsed_json, "params", &params_obj)) {
                struct json_object* filename_obj, *content_obj;
                if (json_object_object_get_ex(params_obj, "filename", &filename_obj) &&
                    json_object_object_get_ex(params_obj, "content", &content_obj)) {
                    const char* filename = json_object_get_string(filename_obj);
                    const char* content = json_object_get_string(content_obj);
                    int result = write_file(filename, content);
                    char response[256];
                    sprintf(response, "{\"result\": %s}", result == 0 ? "true" : "false");
                    send_message_to_frontend(web_view, response);
                }
            }
        } else if (strcmp(method, "listDirectory") == 0) {
            // Handle directory listing request
            struct json_object* params_obj;
            if (json_object_object_get_ex(parsed_json, "params", &params_obj)) {
                struct json_object* dirname_obj;
                if (json_object_object_get_ex(params_obj, "dirname", &dirname_obj)) {
                    const char* dirname = json_object_get_string(dirname_obj);
                    int count;
                    char** files = list_directory(dirname, &count);

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
            }
        }
    }

    json_object_put(parsed_json);
}