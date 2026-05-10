#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pty.h>
#include <utmp.h>
#include <fcntl.h>

// Terminal management for integrated terminal

struct Terminal {
    int master_fd;
    int slave_fd;
    pid_t child_pid;
    char* buffer;
    size_t buffer_size;
};

Terminal* terminal_create() {
    Terminal* term = (Terminal*)malloc(sizeof(Terminal));
    if (!term) return NULL;

    term->buffer = (char*)malloc(4096);
    if (!term->buffer) {
        free(term);
        return NULL;
    }
    term->buffer_size = 4096;

    // Create pseudo-terminal
    if (openpty(&term->master_fd, &term->slave_fd, NULL, NULL, NULL) == -1) {
        free(term->buffer);
        free(term);
        return NULL;
    }

    // Fork process
    term->child_pid = fork();
    if (term->child_pid == -1) {
        close(term->master_fd);
        close(term->slave_fd);
        free(term->buffer);
        free(term);
        return NULL;
    }

    if (term->child_pid == 0) {
        // Child process
        close(term->master_fd);

        // Set up slave as controlling terminal
        setsid();
        ioctl(term->slave_fd, TIOCSCTTY, 0);

        // Redirect stdin, stdout, stderr to slave
        dup2(term->slave_fd, STDIN_FILENO);
        dup2(term->slave_fd, STDOUT_FILENO);
        dup2(term->slave_fd, STDERR_FILENO);

        // Close slave fd
        close(term->slave_fd);

        // Execute shell
        execl("/bin/bash", "bash", NULL);
        exit(1);
    } else {
        // Parent process
        close(term->slave_fd);
    }

    return term;
}

void terminal_destroy(Terminal* term) {
    if (!term) return;

    if (term->child_pid > 0) {
        kill(term->child_pid, SIGTERM);
        waitpid(term->child_pid, NULL, 0);
    }

    if (term->master_fd >= 0) {
        close(term->master_fd);
    }

    free(term->buffer);
    free(term);
}

int terminal_write(Terminal* term, const char* data, size_t size) {
    if (!term || term->master_fd < 0) return -1;
    return write(term->master_fd, data, size);
}

int terminal_read(Terminal* term, char* buffer, size_t size) {
    if (!term || term->master_fd < 0) return -1;

    fd_set read_fds;
    struct timeval timeout = {0, 100000}; // 100ms timeout

    FD_ZERO(&read_fds);
    FD_SET(term->master_fd, &read_fds);

    int result = select(term->master_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result > 0 && FD_ISSET(term->master_fd, &read_fds)) {
        return read(term->master_fd, buffer, size);
    }

    return 0;
}

char* terminal_get_output(Terminal* term) {
    if (!term) return NULL;

    ssize_t bytes_read = terminal_read(term, term->buffer, term->buffer_size - 1);
    if (bytes_read > 0) {
        term->buffer[bytes_read] = '\0';
        return strdup(term->buffer);
    }

    return NULL;
}