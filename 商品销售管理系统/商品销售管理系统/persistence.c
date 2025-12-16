#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "persistence.h"

int loadProductsFromCSV(const char* filename, ProductList* list) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    char line[256];
    int count = 0;
    int maxId = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        Product p;
        char nameBuf[64];
        // Replace unsafe sscanf with sscanf_s for safer parsing
        if (sscanf_s(line, "%d,%63[^,],%lf,%d", &p.id, nameBuf, (unsigned)_countof(nameBuf), &p.price, &p.stock) == 4) 
        {
            if (list->size >= list->capacity) {
                size_t newCap = list->capacity == 0 ? 8 : list->capacity * 2;
                Product* newData = (Product*)realloc(list->data, newCap * sizeof(Product));
                if (!newData) {
                    fclose(fp);
                    return -2;
                }
                list->data = newData;
                list->capacity = newCap;
            }
            Product* dst = &list->data[list->size++];
            dst->id = p.id;
            // Replace unsafe strncpy with safer strncpy_s

strncpy_s(dst->name, sizeof(dst->name), nameBuf, _TRUNCATE);
            dst->name[sizeof(dst->name) - 1] = '\0';
            dst->price = p.price;
            dst->stock = p.stock;
            if (p.id > maxId) maxId = p.id;
            count++;
        }
    }
    fclose(fp);
    list->nextId = maxId + 1;
    return count;
}

int saveProductsToCSV(const char* filename, const ProductList* list) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    fprintf(fp, "#id,name,price,stock\n");
    for (size_t i = 0; i < list->size; ++i) {
        const Product* p = &list->data[i];
        fprintf(fp, "%d,%s,%.2f,%d\n", p->id, p->name, p->price, p->stock);
    }
    fclose(fp);
    return 0;
}

int appendOrderToFile(const char* filename, const Order* order) {
    FILE* fp = fopen(filename, "a");
    if (!fp) return -1;
    fprintf(fp,
        "ORDER,%d,STATUS,%s,ITEMS,%zu,TOTAL,%.2f,CREATED,%ld,PAID,%ld\n",
        order->orderId,
        orderStatusToStr(order->status),
        order->size,
        order->totalAmount,
        (long)order->createdAt,
        (long)order->paidAt);
    for (size_t i = 0; i < order->size; ++i) {
        const OrderItem* it = &order->items[i];
        fprintf(fp, "  ITEM,%d,QTY,%d,UNIT,%.2f,LINE,%.2f\n",
            it->productId, it->quantity, it->unitPrice, it->lineTotal);
    }
    fclose(fp);
    return 0;
}

int loadUsersFromCSV(const char* filename, UserList* ulist) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    char line[256];
    int count = 0;
    int maxId = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        int id;
        char uname[32];
        char pwd[64];
        // Replace unsafe sscanf with sscanf_s for safer parsing
        if (sscanf_s(line, "%d,%31[^,],%63[^\n]", &id, uname, (unsigned)_countof(uname), pwd, (unsigned)_countof(pwd)) == 3)
        {
            if (ulist->size >= ulist->capacity) {
                size_t newCap = ulist->capacity == 0 ? 8 : ulist->capacity * 2;
                User* newData = (User*)realloc(ulist->data, newCap * sizeof(User));
                if (!newData) {
                    fclose(fp);
                    return -2;
                }
                ulist->data = newData;
                ulist->capacity = newCap;
            }
            User* u = &ulist->data[ulist->size++];
            u->id = id;
            strncpy_s(u->username, sizeof(u->username), uname, _TRUNCATE);
            u->username[sizeof(u->username) - 1] = '\0';
            strncpy_s(u->password, sizeof(u->password), pwd, _TRUNCATE);
            u->password[sizeof(u->password) - 1] = '\0';
            if (id > maxId) maxId = id;
            count++;
        }
    }
    fclose(fp);
    ulist->nextId = maxId + 1;
    return count;
}

int saveUsersToCSV(const char* filename, const UserList* ulist) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    fprintf(fp, "#id,username,password\n");
    for (size_t i = 0; i < ulist->size; ++i) {
        const User* u = &ulist->data[i];
        fprintf(fp, "%d,%s,%s\n", u->id, u->username, u->password);
    }
    fclose(fp);
    return 0;
}