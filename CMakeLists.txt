cmake_minimum_required(VERSION 3.10)
project(MagicIDE VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)
pkg_check_modules(JSON REQUIRED json-c)
pkg_check_modules(PCRE REQUIRED libpcre)

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
)

# Create executable
add_executable(magic-ide ${SOURCES})

# Link libraries
target_link_libraries(magic-ide ${GTK_LIBRARIES} ${WEBKIT_LIBRARIES} ${JSON_LIBRARIES} ${PCRE_LIBRARIES})

# Install target
install(TARGETS magic-ide DESTINATION bin)