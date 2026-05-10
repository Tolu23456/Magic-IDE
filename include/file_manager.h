#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <json-c/json.h>

/**
 * @brief File management module for Magic IDE
 *
 * This module provides safe file I/O operations with proper error handling.
 */

/**
 * @brief Read the contents of a file
 *
 * @param filename Path to the file to read
 * @return Allocated string containing file contents, or NULL on error.
 *         Caller must free the returned string.
 */
char* read_file(const char* filename);

/**
 * @brief Write content to a file
 *
 * @param filename Path to the file to write
 * @param content String content to write to the file
 * @return 0 on success, -1 on error
 */
int write_file(const char* filename, const char* content);

/**
 * @brief List files in a directory
 *
 * @param dirname Path to the directory to list
 * @param count Output parameter for number of files returned
 * @return Array of strings containing filenames, or NULL on error.
 *         Caller must free the array using free_file_list().
 */
char** list_directory(const char* dirname, int* count);

/**
 * @brief Free a file list returned by list_directory()
 *
 * @param files Array of filenames to free
 * @param count Number of files in the array
 */
void free_file_list(char** files, int count);

/**
 * @brief Check if a file exists
 *
 * @param filename Path to check
 * @return 1 if file exists, 0 otherwise
 */
int file_exists(const char* filename);

/**
 * @brief Check if a path is a directory
 *
 * @param path Path to check
 * @return 1 if path is a directory, 0 otherwise
 */
int is_directory(const char* path);

#endif // FILE_MANAGER_H