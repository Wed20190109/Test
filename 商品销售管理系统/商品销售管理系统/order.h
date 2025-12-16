#ifndef ORDER_H
#define ORDER_H

#include <time.h>
#include "product.h"

typedef enum {
    ORDER_CREATED = 0,
    ORDER_PAID,
    ORDER_CANCELLED
} OrderStatus;

typedef struct {
    int productId;
    int quantity;
    double unitPrice;
    double lineTotal;
} OrderItem;

typedef struct {
    int        orderId;
    OrderItem* items;
    size_t     size;
    size_t     capacity;
    double     totalAmount;
    OrderStatus status;
    time_t     createdAt;
    time_t     paidAt;      // 0 if not paid
} Order;

void initOrder(Order* order, int orderId);
void freeOrder(Order* order);
int  addOrderItem(Order* order, const Product* p, int quantity);
void printOrder(const Order* order);
void markOrderPaid(Order* order);
void cancelOrder(Order* order);

const char* orderStatusToStr(OrderStatus st);

#endif