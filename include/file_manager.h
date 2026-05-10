#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <json-c/json.h>

// File management function declarations
char* read_file(const char* filename);
int write_file(const char* filename, const char* content);
char** list_directory(const char* dirname, int* count);
void free_file_list(char** files, int count);
int file_exists(const char* filename);
int is_directory(const char* path);

#endif // FILE_MANAGER_H