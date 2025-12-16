#pragma once
#ifndef PRODUCT_H
#define PRODUCT_H

#include <stddef.h>

typedef struct {
    int    id;
    char   name[64];
    double price;
    int    stock;
} Product;

typedef struct {
    Product* data;
    size_t   size;
    size_t   capacity;
    int      nextId;   // 新增：保证ID单调递增
} ProductList;

void initProductList(ProductList* list);
void freeProductList(ProductList* list);
int  addProduct(ProductList* list, const char* name, double price, int stock);
Product* findProductById(ProductList* list, int id);
void listProducts(const ProductList* list);

// 新增功能
int modifyProduct(ProductList* list, int id, const char* name, double price, int stock);
int deleteProduct(ProductList* list, int id); // 成功返回0，失败返回-1

#endif