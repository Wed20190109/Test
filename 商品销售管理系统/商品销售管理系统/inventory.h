#ifndef INVENTORY_H
#define INVENTORY_H

#include "product.h"

int deductStock(Product* p, int qty);     // 成功返回0，库存不足返回-1
int increaseStock(Product* p, int qty);   // 成功返回0
#endif