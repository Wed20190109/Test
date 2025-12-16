#include "purchase.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ensureCap(PurchaseList* list) {
    if (list->size >= list->capacity) {
        size_t newCap = (list->capacity == 0) ? 16 : list->capacity * 2;
        Purchase* nd = (Purchase*)realloc(list->data, newCap * sizeof(Purchase));
        if (!nd) {
            fprintf(stderr, "Purchase list realloc failed\n");
            exit(EXIT_FAILURE);
        }
        list->data = nd;
        list->capacity = newCap;
    }
}

void initPurchaseList(PurchaseList* list) {
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}

void freePurchaseList(PurchaseList* list) {
    if (!list) return;
    free(list->data);
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}

Purchase* addPurchase(PurchaseList* list,
    int purchaseId,
    int productId,
    int quantity,
    double unitCost,
    long long createdAt) {
    ensureCap(list);
    Purchase* p = &list->data[list->size++];
    p->purchaseId = purchaseId;
    p->productId = productId;
    p->quantity = quantity;
    p->unitCost = unitCost;
    p->createdAt = createdAt;
    return p;
}

int appendPurchaseToCSV(const char* path, const Purchase* p) {
    FILE* fp = fopen(path, "a");
    if (!fp) return -1;
    fprintf(fp, "%d,%d,%d,%.6f,%lld\n",
        p->purchaseId, p->productId, p->quantity, p->unitCost, p->createdAt);
    fclose(fp);
    return 0;
}

static void trim_eol(char* s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

/* very simple 5-field CSV */
static int parse_line(const char* line, Purchase* out) {
    char buf[256];
    strncpy_s(buf, sizeof(buf), line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim_eol(buf);

    char* context = NULL;
    char* tok = strtok_s(buf, ",", &context);
    if (!tok) return 0;
    out->purchaseId = atoi(tok);

    tok = strtok_s(NULL, ",", &context);
    if (!tok) return 0;
    out->productId = atoi(tok);

    tok = strtok_s(NULL, ",", &context);
    if (!tok) return 0;
    out->quantity = atoi(tok);

    tok = strtok_s(NULL, ",", &context);
    if (!tok) return 0;
    out->unitCost = atof(tok);

    tok = strtok_s(NULL, ",", &context);
    if (!tok) return 0;
    out->createdAt = strtoll(tok, NULL, 10);

    return 1;
}

int loadPurchasesFromCSV(const char* path, PurchaseList* out) {
    FILE* fp = fopen(path, "r");
    if (!fp) return -1;

    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        Purchase p;
        if (parse_line(line, &p)) {
            addPurchase(out, p.purchaseId, p.productId, p.quantity, p.unitCost, p.createdAt);
            count++;
        }
    }
    fclose(fp);
    return count;
}

int purchase_nextIdFromList(const PurchaseList* list) {
    int maxId = 0;
    if (!list) return 1;
    for (size_t i = 0; i < list->size; ++i) {
        if (list->data[i].purchaseId > maxId) maxId = list->data[i].purchaseId;
    }
    return maxId + 1;
}

void purchase_printLog(const PurchaseList* list) {
    if (!list || list->size == 0) {
        printf("No purchases.\n");
        return;
    }
    printf("=== Purchase Log ===\n");
    printf("%-6s %-8s %-8s %-10s %-12s\n", "PID", "ProdID", "Qty", "UnitCost", "CreatedAt");
    for (size_t i = 0; i < list->size; ++i) {
        const Purchase* p = &list->data[i];
        printf("%-6d %-8d %-8d %-10.2f %-12lld\n",
            p->purchaseId, p->productId, p->quantity, p->unitCost, p->createdAt);
    }
}

typedef struct {
    int productId;
    long long totalQty;
    double totalCost;
} Agg;

static void agg_add(Agg** arr, size_t* n, size_t* cap, int productId, int qty, double unitCost) {
    for (size_t i = 0; i < *n; ++i) {
        if ((*arr)[i].productId == productId) {
            (*arr)[i].totalQty += qty;
            (*arr)[i].totalCost += (double)qty * unitCost;
            return;
        }
    }
    if (*n >= *cap) {
        size_t newCap = (*cap == 0) ? 16 : (*cap * 2);
        Agg* nd = (Agg*)realloc(*arr, newCap * sizeof(Agg));
        if (!nd) return;
        *arr = nd;
        *cap = newCap;
    }
    (*arr)[*n].productId = productId;
    (*arr)[*n].totalQty = qty;
    (*arr)[*n].totalCost = (double)qty * unitCost;
    (*n)++;
}

static int cmp_qty_desc(const void* a, const void* b) {
    const Agg* x = (const Agg*)a;
    const Agg* y = (const Agg*)b;
    if (x->totalQty < y->totalQty) return 1;
    if (x->totalQty > y->totalQty) return -1;
    return 0;
}

void purchase_printSummaryByProduct(const PurchaseList* list) {
    if (!list || list->size == 0) {
        printf("No purchases.\n");
        return;
    }

    Agg* ag = NULL;
    size_t n = 0, cap = 0;
    for (size_t i = 0; i < list->size; ++i) {
        const Purchase* p = &list->data[i];
        agg_add(&ag, &n, &cap, p->productId, p->quantity, p->unitCost);
    }
    qsort(ag, n, sizeof(Agg), cmp_qty_desc);

    printf("=== Purchase Summary By Product ===\n");
    printf("%-8s %-10s %-12s %-12s\n", "ProdID", "TotalQty", "TotalCost", "AvgCost");
    for (size_t i = 0; i < n; ++i) {
        double avg = (ag[i].totalQty > 0) ? (ag[i].totalCost / (double)ag[i].totalQty) : 0.0;
        printf("%-8d %-10lld %-12.2f %-12.2f\n",
            ag[i].productId, ag[i].totalQty, ag[i].totalCost, avg);
    }
    free(ag);
}