#include "lsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <json-c/json.h>

// Language Server Protocol client

struct LSPClient {
    int socket_fd;
    char* server_command;
    pid_t server_pid;
    char* buffer;
    size_t buffer_size;
};

LSPClient* lsp_create(const char* language) {
    LSPClient* client = (LSPClient*)malloc(sizeof(LSPClient));
    if (!client) return NULL;

    client->buffer = (char*)malloc(8192);
    if (!client->buffer) {
        free(client);
        return NULL;
    }
    client->buffer_size = 8192;
    client->socket_fd = -1;
    client->server_pid = -1;

    // Determine language server command based on language
    if (strcmp(language, "javascript") == 0 || strcmp(language, "typescript") == 0) {
        client->server_command = strdup("typescript-language-server --stdio");
    } else if (strcmp(language, "python") == 0) {
        client->server_command = strdup("python -m pyright-langserver --stdio");
    } else if (strcmp(language, "c") == 0 || strcmp(language, "cpp") == 0) {
        client->server_command = strdup("clangd");
    } else {
        // Generic fallback
        client->server_command = NULL;
    }

    return client;
}

void lsp_destroy(LSPClient* client) {
    if (!client) return;

    if (client->socket_fd >= 0) {
        close(client->socket_fd);
    }

    if (client->server_pid > 0) {
        kill(client->server_pid, SIGTERM);
    }

    free(client->server_command);
    free(client->buffer);
    free(client);
}

int lsp_start_server(LSPClient* client) {
    if (!client || !client->server_command) return -1;

    int pipe_stdin[2], pipe_stdout[2];

    if (pipe(pipe_stdin) == -1 || pipe(pipe_stdout) == -1) {
        return -1;
    }

    client->server_pid = fork();
    if (client->server_pid == -1) {
        close(pipe_stdin[0]); close(pipe_stdin[1]);
        close(pipe_stdout[0]); close(pipe_stdout[1]);
        return -1;
    }

    if (client->server_pid == 0) {
        // Child process
        dup2(pipe_stdin[0], STDIN_FILENO);
        dup2(pipe_stdout[1], STDOUT_FILENO);

        close(pipe_stdin[0]); close(pipe_stdin[1]);
        close(pipe_stdout[0]); close(pipe_stdout[1]);

        execl("/bin/sh", "sh", "-c", client->server_command, NULL);
        exit(1);
    } else {
        // Parent process
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);

        client->socket_fd = pipe_stdout[0]; // Use pipe as socket for simplicity
    }

    return 0;
}

int lsp_send_request(LSPClient* client, const char* request) {
    if (!client || client->socket_fd < 0) return -1;

    // LSP uses JSON-RPC 2.0 with Content-Length header
    char header[256];
    sprintf(header, "Content-Length: %zu\r\n\r\n", strlen(request));

    if (write(client->socket_fd, header, strlen(header)) == -1) {
        return -1;
    }

    return write(client->socket_fd, request, strlen(request));
}

char* lsp_receive_response(LSPClient* client) {
    if (!client || client->socket_fd < 0) return NULL;

    // Read Content-Length header
    char header[256];
    size_t header_pos = 0;

    while (header_pos < sizeof(header) - 1) {
        char c;
        if (read(client->socket_fd, &c, 1) <= 0) return NULL;

        header[header_pos++] = c;
        if (header_pos >= 4 && strncmp(&header[header_pos - 4], "\r\n\r\n", 4) == 0) {
            break;
        }
    }
    header[header_pos] = '\0';

    // Parse Content-Length
    char* content_length_str = strstr(header, "Content-Length:");
    if (!content_length_str) return NULL;

    int content_length = atoi(content_length_str + 15);

    if (content_length > client->buffer_size - 1) {
        client->buffer = (char*)realloc(client->buffer, content_length + 1);
        client->buffer_size = content_length + 1;
    }

    // Read content
    size_t total_read = 0;
    while (total_read < content_length) {
        ssize_t bytes_read = read(client->socket_fd, client->buffer + total_read, content_length - total_read);
        if (bytes_read <= 0) return NULL;
        total_read += bytes_read;
    }
    client->buffer[total_read] = '\0';

    return strdup(client->buffer);
}