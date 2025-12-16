#pragma once
#pragma once
#ifndef PURCHASE_H
#define PURCHASE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        int purchaseId;
        int productId;
        int quantity;
        double unitCost;
        long long createdAt; /* epoch seconds */
    } Purchase;

    typedef struct {
        Purchase* data;
        size_t size;
        size_t capacity;
    } PurchaseList;

    void initPurchaseList(PurchaseList* list);
    void freePurchaseList(PurchaseList* list);

    Purchase* addPurchase(PurchaseList* list,
        int purchaseId,
        int productId,
        int quantity,
        double unitCost,
        long long createdAt);

    /* CSV schema: purchaseId,productId,quantity,unitCost,createdAt */
    int appendPurchaseToCSV(const char* path, const Purchase* p);
    int loadPurchasesFromCSV(const char* path, PurchaseList* out);

    int purchase_nextIdFromList(const PurchaseList* list);

    /* Êä³ö */
    void purchase_printLog(const PurchaseList* list);
    void purchase_printSummaryByProduct(const PurchaseList* list);

#ifdef __cplusplus
}
#endif

#endif