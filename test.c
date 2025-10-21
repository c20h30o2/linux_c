#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

void readline(char *buf, size_t size);

void readline(char *buf, size_t size) {
    if (fgets(buf, size, stdin) == NULL) {
        perror("error reading input");
        exit(EXIT_FAILURE);
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0'; // Remove newline character
    }
}