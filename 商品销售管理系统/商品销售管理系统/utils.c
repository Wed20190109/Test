#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"

int readInt(const char* prompt) {
    char buf[64];
    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    return atoi(buf);
}

double readDouble(const char* prompt) {
    char buf[64];
    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) return 0.0;
    return atof(buf);
}

void readLine(const char* prompt, char* buf, int size) {
    printf("%s", prompt);
    if (fgets(buf, size, stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    }
    else {
        if (size > 0) buf[0] = '\0';
    }
}