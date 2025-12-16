#pragma once
#ifndef USER_H
#define USER_H

#include <stddef.h>

typedef struct {
    int  id;
    char username[32];
    char password[64]; // 演示用明文，生产环境需哈希
} User;

typedef struct {
    User* data;
    size_t  size;
    size_t  capacity;
    int     nextId;
} UserList;

void initUserList(UserList* list);
void freeUserList(UserList* list);
User* registerUser(UserList* list, const char* username, const char* password);
User* authenticate(UserList* list, const char* username, const char* password);
User* findUserByName(UserList* list, const char* username);
void listUsers(const UserList* list); // 可选展示

#endif