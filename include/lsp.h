#ifndef LSP_H
#define LSP_H

#include <json-c/json.h>

typedef struct LSPClient LSPClient;

// LSP function declarations
LSPClient* lsp_create(const char* language);
void lsp_destroy(LSPClient* client);
int lsp_start_server(LSPClient* client);
int lsp_send_request(LSPClient* client, const char* request);
char* lsp_receive_response(LSPClient* client);

#endif // LSP_H