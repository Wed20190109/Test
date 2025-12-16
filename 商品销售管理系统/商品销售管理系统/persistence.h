#pragma once
#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "product.h"
#include "order.h"
#include "user.h"

int loadProductsFromCSV(const char* filename, ProductList* list);
int saveProductsToCSV(const char* filename, const ProductList* list);

int appendOrderToFile(const char* filename, const Order* order);

int loadUsersFromCSV(const char* filename, UserList* ulist);
int saveUsersToCSV(const char* filename, const UserList* ulist);

#endif