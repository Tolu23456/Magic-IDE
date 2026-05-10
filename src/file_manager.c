#include "file_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// File management functions

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

    return buffer;
}

int write_file(const char* filename, const char* content) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        return -1;
    }

    size_t written = fwrite(content, 1, strlen(content), file);
    fclose(file);

    return written == strlen(content) ? 0 : -1;
}

char** list_directory(const char* dirname, int* count) {
    DIR* dir = opendir(dirname);
    if (!dir) {
        *count = 0;
        return NULL;
    }

    char** files = NULL;
    *count = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        files = (char**)realloc(files, (*count + 1) * sizeof(char*));
        files[*count] = strdup(entry->d_name);
        (*count)++;
    }

    closedir(dir);
    return files;
}

void free_file_list(char** files, int count) {
    for (int i = 0; i < count; i++) {
        free(files[i]);
    }
    free(files);
}

int file_exists(const char* filename) {
    return access(filename, F_OK) == 0;
}

int is_directory(const char* path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}