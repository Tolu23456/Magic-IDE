#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/file_manager.h"

// Simple test framework for Magic IDE

#define TEST_PASS 0
#define TEST_FAIL 1

typedef struct {
    const char* name;
    int (*test_func)(void);
} TestCase;

static int tests_run = 0;
static int tests_passed = 0;

// Test file reading
static int test_read_file(void) {
    // Create a temporary test file
    const char* test_content = "Hello, World!\nThis is a test file.";
    const char* test_filename = "/tmp/magic_ide_test.txt";

    // Write test content
    FILE* file = fopen(test_filename, "w");
    if (!file) return TEST_FAIL;
    fwrite(test_content, 1, strlen(test_content), file);
    fclose(file);

    // Read it back
    char* content = read_file(test_filename);
    if (!content) return TEST_FAIL;

    int result = strcmp(content, test_content) == 0 ? TEST_PASS : TEST_FAIL;
    free(content);

    // Clean up
    remove(test_filename);

    return result;
}

// Test file writing
static int test_write_file(void) {
    const char* test_content = "Test content for writing.";
    const char* test_filename = "/tmp/magic_ide_write_test.txt";

    // Write file
    int write_result = write_file(test_filename, test_content);
    if (write_result != 0) return TEST_FAIL;

    // Read it back to verify
    char* content = read_file(test_filename);
    if (!content) return TEST_FAIL;

    int result = strcmp(content, test_content) == 0 ? TEST_PASS : TEST_FAIL;
    free(content);

    // Clean up
    remove(test_filename);

    return result;
}

// Test file existence
static int test_file_exists(void) {
    const char* test_filename = "/tmp/magic_ide_exists_test.txt";

    // File shouldn't exist initially
    if (file_exists(test_filename)) return TEST_FAIL;

    // Create file
    FILE* file = fopen(test_filename, "w");
    if (!file) return TEST_FAIL;
    fclose(file);

    // File should exist now
    if (!file_exists(test_filename)) return TEST_FAIL;

    // Clean up
    remove(test_filename);

    return TEST_PASS;
}

// Test directory listing
static int test_list_directory(void) {
    const char* test_dir = "/tmp/magic_ide_dir_test";

    // Create test directory
    if (mkdir(test_dir, 0755) != 0) return TEST_FAIL;

    // Create some test files
    FILE* file1 = fopen("/tmp/magic_ide_dir_test/file1.txt", "w");
    FILE* file2 = fopen("/tmp/magic_ide_dir_test/file2.txt", "w");
    if (!file1 || !file2) {
        rmdir(test_dir);
        return TEST_FAIL;
    }
    fclose(file1);
    fclose(file2);

    // List directory
    int count;
    char** files = list_directory(test_dir, &count);

    if (!files || count != 2) {
        if (files) free_file_list(files, count);
        remove("/tmp/magic_ide_dir_test/file1.txt");
        remove("/tmp/magic_ide_dir_test/file2.txt");
        rmdir(test_dir);
        return TEST_FAIL;
    }

    free_file_list(files, count);

    // Clean up
    remove("/tmp/magic_ide_dir_test/file1.txt");
    remove("/tmp/magic_ide_dir_test/file2.txt");
    rmdir(test_dir);

    return TEST_PASS;
}

// Run a single test
static void run_test(const TestCase* test) {
    printf("Running test: %s... ", test->name);
    fflush(stdout);

    int result = test->test_func();
    tests_run++;

    if (result == TEST_PASS) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
}

int main(void) {
    printf("Magic IDE Test Suite\n");
    printf("===================\n\n");

    // Define test cases
    TestCase tests[] = {
        {"read_file", test_read_file},
        {"write_file", test_write_file},
        {"file_exists", test_file_exists},
        {"list_directory", test_list_directory},
        {NULL, NULL} // Sentinel
    };

    // Run all tests
    for (int i = 0; tests[i].name != NULL; i++) {
        run_test(&tests[i]);
    }

    printf("\nTest Results: %d/%d tests passed\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}