#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "product.h"

void initProductList(ProductList* list) {
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
    list->nextId = 1;
}

void freeProductList(ProductList* list) {
    free(list->data);
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
    list->nextId = 1;
}

static void ensureCapacity(ProductList* list) {
    if (list->size >= list->capacity) {
        size_t newCap = list->capacity == 0 ? 8 : list->capacity * 2;
        Product* newData = (Product*)realloc(list->data, newCap * sizeof(Product));
        if (!newData) {
            fprintf(stderr, "Memory expansion failed\n");
            exit(EXIT_FAILURE);
        }
        list->data = newData;
        list->capacity = newCap;
    }
}

int addProduct(ProductList* list, const char* name, double price, int stock) {
    if (!name || price < 0 || stock < 0) return -1;
    ensureCapacity(list);
    Product* p = &list->data[list->size++];
    p->id = list->nextId++;
    strncpy_s(p->name, sizeof(p->name), name, _TRUNCATE);
    p->name[sizeof(p->name) - 1] = '\0';
    p->price = price;
    p->stock = stock;
    return p->id;
}

Product* findProductById(ProductList* list, int id) {
    for (size_t i = 0; i < list->size; ++i) {
        if (list->data[i].id == id) return &list->data[i];
    }
    return NULL;
}

void listProducts(const ProductList* list) {
    printf("=== Product List ===\n");
    printf("%-5s %-20s %-10s %-10s\n", "ID", "Name", "Price", "Stock");
    for (size_t i = 0; i < list->size; ++i) {
        const Product* p = &list->data[i];
        printf("%-5d %-20s %-10.2f %-10d\n", p->id, p->name, p->price, p->stock);
    }
}

int modifyProduct(ProductList* list, int id, const char* name, double price, int stock) {
    Product* p = findProductById(list, id);
    if (!p) return -1;
    if (name && *name) 
    {
        strncpy_s(p->name, sizeof(p->name), name, _TRUNCATE);
        p->name[sizeof(p->name) - 1] = '\0';
    }
    if (price >= 0) p->price = price;
    if (stock >= 0) p->stock = stock;
    return 0;
}

int deleteProduct(ProductList* list, int id) {
    for (size_t i = 0; i < list->size; ++i) {
        if (list->data[i].id == id) {
            for (size_t j = i + 1; j < list->size; ++j) {
                list->data[j - 1] = list->data[j];
            }
            list->size--;
            return 0;
        }
    }
    return -1;
}