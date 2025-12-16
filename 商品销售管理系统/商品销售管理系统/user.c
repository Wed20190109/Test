#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"

void initUserList(UserList* list) {
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
    list->nextId = 1;
}

void freeUserList(UserList* list) {
    free(list->data);
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
    list->nextId = 1;
}

static void ensureUserCapacity(UserList* list) {
    if (list->size >= list->capacity) {
        size_t newCap = list->capacity == 0 ? 8 : list->capacity * 2;
        User* newData = (User*)realloc(list->data, newCap * sizeof(User));
        if (!newData) {
            fprintf(stderr, "User memory expansion failed\n");
            exit(EXIT_FAILURE);
        }
        list->data = newData;
        list->capacity = newCap;
    }
}

User* findUserByName(UserList* list, const char* username) {
    for (size_t i = 0; i < list->size; ++i) {
        if (strcmp(list->data[i].username, username) == 0) {
            return &list->data[i];
        }
    }
    return NULL;
}

User* registerUser(UserList* list, const char* username, const char* password) {
    if (!username || !password || !*username || !*password) return NULL;
    if (findUserByName(list, username)) {
        return NULL; // Already exists
    }
    ensureUserCapacity(list);
    User* u = &list->data[list->size++];
    u->id = list->nextId++;
    strncpy_s(u->username, sizeof(u->username), username, _TRUNCATE);
    u->username[sizeof(u->username) - 1] = '\0';
    strncpy_s(u->password, sizeof(u->password), password, _TRUNCATE);
    u->password[sizeof(u->password) - 1] = '\0';
    return u;
}

User* authenticate(UserList* list, const char* username, const char* password) {
    User* u = findUserByName(list, username);
    if (u && strcmp(u->password, password) == 0) {
        return u;
    }
    return NULL;
}

void listUsers(const UserList* list) {
    printf("=== User List (demo) ===\n");
    printf("%-5s %-16s\n", "ID", "Username");
    for (size_t i = 0; i < list->size; ++i) {
        printf("%-5d %-16s\n", list->data[i].id, list->data[i].username);
    }
}