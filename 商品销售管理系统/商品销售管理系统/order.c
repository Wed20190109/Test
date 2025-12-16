#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "order.h"

void initOrder(Order* order, int orderId) {
    order->orderId = orderId;
    order->items = NULL;
    order->size = 0;
    order->capacity = 0;
    order->totalAmount = 0.0;
    order->status = ORDER_CREATED;
    order->createdAt = time(NULL);
    order->paidAt = 0;
}

void freeOrder(Order* order) {
    free(order->items);
    order->items = NULL;
    order->size = 0;
    order->capacity = 0;
    order->totalAmount = 0.0;
    order->status = ORDER_CANCELLED; // 释放后不再使用
    order->createdAt = 0;
    order->paidAt = 0;
}

static void ensureItemCapacity(Order* order) {
    if (order->size >= order->capacity) {
        size_t newCap = order->capacity == 0 ? 4 : order->capacity * 2;
        OrderItem* newItems = (OrderItem*)realloc(order->items, newCap * sizeof(OrderItem));
        if (!newItems) {
            fprintf(stderr, "The order memory expansion failed\n");
            exit(EXIT_FAILURE);
        }
        order->items = newItems;
        order->capacity = newCap;
    }
}

int addOrderItem(Order* order, const Product* p, int quantity) {
    if (!order || !p || quantity <= 0) return -1;
    ensureItemCapacity(order);
    OrderItem* item = &order->items[order->size++];
    item->productId = p->id;
    item->quantity = quantity;
    item->unitPrice = p->price;
    item->lineTotal = p->price * quantity;
    order->totalAmount += item->lineTotal;
    return 0;
}

const char* orderStatusToStr(OrderStatus st) {
    switch (st) {
    case ORDER_CREATED: return "CREATED";
    case ORDER_PAID: return "PAID";
    case ORDER_CANCELLED: return "CANCELLED";
    default: return "UNKNOWN";
    }
}

void markOrderPaid(Order* order) {
    if (order->status == ORDER_CREATED) {
        order->status = ORDER_PAID;
        order->paidAt = time(NULL);
    }
}

void cancelOrder(Order* order) {
    if (order->status == ORDER_CREATED) {
        order->status = ORDER_CANCELLED;
    }
}

static void printTime(time_t t) {
    if (t == 0) {
        printf("-");
        return;
    }
    char buf[32];
    struct tm tmv;
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    printf("%s", buf);
}

void printOrder(const Order* order) {
    printf("=== Order #%d Details ===\n", order->orderId);
    printf("Status: %s\n", orderStatusToStr(order->status));
    printf("Creation time: "); printTime(order->createdAt); printf("\n");
    printf("Payment time: "); printTime(order->paidAt); printf("\n");
    printf("%-8s %-8s %-10s %-10s\n", "ProdID", "Quantity", "Unit price", "subtotal");
    for (size_t i = 0; i < order->size; ++i) {
        const OrderItem* item = &order->items[i];
        printf("%-8d %-8d %-10.2f %-10.2f\n",
            item->productId, item->quantity, item->unitPrice, item->lineTotal);
    }
    printf("Total amount: %.2f\n", order->totalAmount);
}