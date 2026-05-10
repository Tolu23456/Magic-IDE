#include "file_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// File management functions

char* read_file(const char* filename) {
    if (!filename) {
        return NULL;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file for reading: %s\n", filename);
        return NULL;
    }

    // Get file size
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Failed to seek to end of file: %s\n", filename);
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fprintf(stderr, "Failed to get file size: %s\n", filename);
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Failed to seek to beginning of file: %s\n", filename);
        fclose(file);
        return NULL;
    }

    // Allocate buffer (add 1 for null terminator)
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for file content\n");
        fclose(file);
        return NULL;
    }

    // Read file content
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Failed to read complete file: %s\n", filename);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size] = '\0';
    fclose(file);

    return buffer;
}

int write_file(const char* filename, const char* content) {
    if (!filename || !content) {
        fprintf(stderr, "Invalid parameters for write_file\n");
        return -1;
    }

    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing: %s\n", filename);
        return -1;
    }

    size_t content_length = strlen(content);
    size_t bytes_written = fwrite(content, 1, content_length, file);

    if (bytes_written != content_length) {
        fprintf(stderr, "Failed to write complete content to file: %s\n", filename);
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Failed to close file: %s\n", filename);
        return -1;
    }

    return 0;
}

char** list_directory(const char* dirname, int* count) {
    if (!dirname || !count) {
        fprintf(stderr, "Invalid parameters for list_directory\n");
        *count = 0;
        return NULL;
    }

    DIR* dir = opendir(dirname);
    if (!dir) {
        fprintf(stderr, "Failed to open directory: %s\n", dirname);
        *count = -1;
        return NULL;
    }

    char** files = NULL;
    *count = 0;
    struct dirent* entry;

    // First pass: count entries
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        (*count)++;
    }

    // Reset directory stream
    rewinddir(dir);

    if (*count == 0) {
        closedir(dir);
        return NULL;
    }

    // Allocate array for filenames
    files = (char**)malloc(*count * sizeof(char*));
    if (!files) {
        fprintf(stderr, "Failed to allocate memory for file list\n");
        closedir(dir);
        *count = -1;
        return NULL;
    }

    // Second pass: store filenames
    int index = 0;
    while ((entry = readdir(dir)) != NULL && index < *count) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        files[index] = strdup(entry->d_name);
        if (!files[index]) {
            fprintf(stderr, "Failed to duplicate filename\n");
            // Clean up allocated memory
            for (int i = 0; i < index; i++) {
                free(files[i]);
            }
            free(files);
            closedir(dir);
            *count = -1;
            return NULL;
        }
        index++;
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