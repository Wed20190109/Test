#include "inventory.h"



int deductStock(Product* p, int qty) {
    if (!p || qty <= 0) return -1;
    if (p->stock < qty) return -1;
    p->stock -= qty;
    return 0;
}

int increaseStock(Product* p, int qty) {
    if (!p || qty <= 0) return -1;
    p->stock += qty;
    return 0;
}