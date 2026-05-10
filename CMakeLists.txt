cmake_minimum_required(VERSION 3.10)
project(MagicIDE VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable warnings and treat them as errors in debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)
pkg_check_modules(JSON REQUIRED json-c)
pkg_check_modules(PCRE REQUIRED libpcre)

# Find Threads package for threading support
find_package(Threads REQUIRED)

# Include directories
include_directories(${GTK_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS} ${JSON_INCLUDE_DIRS} ${PCRE_INCLUDE_DIRS})
include_directories(include)

# Link directories
link_directories(${GTK_LIBRARY_DIRS} ${WEBKIT_LIBRARY_DIRS} ${JSON_LIBRARY_DIRS} ${PCRE_LIBRARY_DIRS})

# Add compile options
add_compile_options(${GTK_CFLAGS_OTHER} ${WEBKIT_CFLAGS_OTHER} ${JSON_CFLAGS_OTHER} ${PCRE_CFLAGS_OTHER})

# Source files
set(SOURCES
    src/main.c
    src/webview.c
    src/file_manager.c
    src/ipc.c
    src/terminal.c
    src/lsp.c
    src/logger.c
    src/config.c
    src/memory_pool.c
    src/plugin_manager.c
    src/async_ipc.c
)

# Create executable
add_executable(magic-ide ${SOURCES})

# Link libraries
target_link_libraries(magic-ide ${GTK_LIBRARIES} ${WEBKIT_LIBRARIES} ${JSON_LIBRARIES} ${PCRE_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS})

# Install target
install(TARGETS magic-ide DESTINATION bin)

# Add custom target for running tests (if they exist)
add_custom_target(test
    COMMAND ${CMAKE_BINARY_DIR}/test_file_manager
    DEPENDS test_file_manager
    COMMENT "Running tests"
)

# Build test executable
add_executable(test_file_manager tests/test_file_manager.c src/file_manager.c)
target_include_directories(test_file_manager PRIVATE include)
target_link_libraries(test_file_manager ${JSON_LIBRARIES})

# Add custom target for code formatting (requires clang-format)
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${SOURCES}
        COMMENT "Formatting source code"
    )
endif()