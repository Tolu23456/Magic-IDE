#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

typedef struct Terminal Terminal;

// Terminal function declarations
Terminal* terminal_create();
void terminal_destroy(Terminal* term);
int terminal_write(Terminal* term, const char* data, size_t size);
int terminal_read(Terminal* term, char* buffer, size_t size);
char* terminal_get_output(Terminal* term);

#endif // TERMINAL_H