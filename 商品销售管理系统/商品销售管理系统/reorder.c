#include "reorder.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ensureCap(ReorderTable* t) {
    if (t->size >= t->capacity) {
        size_t newCap = (t->capacity == 0) ? 16 : t->capacity * 2;
        ReorderLevel* nd = (ReorderLevel*)realloc(t->data, newCap * sizeof(ReorderLevel));
        if (!nd) {
            fprintf(stderr, "reorder table realloc failed\n");
            exit(EXIT_FAILURE);
        }
        t->data = nd;
        t->capacity = newCap;
    }
}

void reorder_init(ReorderTable* t) {
    t->data = NULL;
    t->size = 0;
    t->capacity = 0;
}

void reorder_free(ReorderTable* t) {
    if (!t) return;
    free(t->data);
    t->data = NULL;
    t->size = 0;
    t->capacity = 0;
}

static void trim_eol(char* s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

int reorder_loadCSV(const char* path, ReorderTable* t) {
    FILE* fp = fopen(path, "r");
    if (!fp) return -1;

    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim_eol(line);
        if (line[0] == '\0') continue;

        char* next_token = NULL;
        char* p = strtok_s(line, ",", &next_token);
        if (!p) continue;
        int pid = atoi(p);

        p = strtok_s(NULL, ",", &next_token);
        if (!p) continue;
        int lvl = atoi(p);

        /* upsert */
        reorder_setLevel(t, pid, lvl);
        count++;
    }

    fclose(fp);
    return count;
}

int reorder_saveCSV(const char* path, const ReorderTable* t) {
    FILE* fp = fopen(path, "w");
    if (!fp) return -1;

    for (size_t i = 0; i < t->size; ++i) {
        fprintf(fp, "%d,%d\n", t->data[i].productId, t->data[i].reorderLevel);
    }
    fclose(fp);
    return 0;
}

int reorder_getLevel(const ReorderTable* t, int productId, int defaultLevel) {
    if (!t) return defaultLevel;
    for (size_t i = 0; i < t->size; ++i) {
        if (t->data[i].productId == productId) return t->data[i].reorderLevel;
    }
    return defaultLevel;
}

void reorder_setLevel(ReorderTable* t, int productId, int level) {
    for (size_t i = 0; i < t->size; ++i) {
        if (t->data[i].productId == productId) {
            t->data[i].reorderLevel = level;
            return;
        }
    }
    ensureCap(t);
    t->data[t->size].productId = productId;
    t->data[t->size].reorderLevel = level;
    t->size++;
}

void reorder_remove(ReorderTable* t, int productId) {
    if (!t) return;
    for (size_t i = 0; i < t->size; ++i) {
        if (t->data[i].productId == productId) {
            t->data[i] = t->data[t->size - 1];
            t->size--;
            return;
        }
    }
}

void reorder_printLowStock(const ProductList* products,
    const ReorderTable* t,
    int defaultLevel) {
    printf("\n=== Low Stock Alert ===\n");
    printf("%-6s %-20s %-8s %-8s\n", "ID", "Name", "Stock", "Level");

    int any = 0;
    for (size_t i = 0; i < products->size; ++i) {
        const Product* p = &products->data[i];
        int lvl = reorder_getLevel(t, p->id, defaultLevel);
        if (p->stock < lvl) {
            any = 1;
            printf("%-6d %-20s %-8d %-8d\n", p->id, p->name, p->stock, lvl);
        }
    }
    if (!any) printf("All stocks are above reorder levels.\n");
}

/* 补货建议：建议数量 = level - stock（若为正） */
void reorder_printReplenishList(const ProductList* products,
    const ReorderTable* t,
    int defaultLevel) {
    printf("\n=== Replenish Suggestion List ===\n");
    printf("%-6s %-20s %-8s %-8s %-10s\n", "ID", "Name", "Stock", "Level", "Suggest");

    int any = 0;
    for (size_t i = 0; i < products->size; ++i) {
        const Product* p = &products->data[i];
        int lvl = reorder_getLevel(t, p->id, defaultLevel);
        int need = lvl - p->stock;
        if (need > 0) {
            any = 1;
            printf("%-6d %-20s %-8d %-8d %-10d\n", p->id, p->name, p->stock, lvl, need);
        }
    }
    if (!any) printf("No replenishment needed.\n");
}

void reorder_interactiveSetLevel(const char* csvPath,
    ReorderTable* t,
    ProductList* products,
    int defaultLevel) {
    int pid = readInt("Product ID: ");
    Product* p = findProductById(products, pid);
    if (!p) {
        printf("Product not found.\n");
        return;
    }
    printf("Product: %s (current stock=%d)\n", p->name, p->stock);

    int cur = reorder_getLevel(t, pid, defaultLevel);
    printf("Current reorder level: %d\n", cur);

    int lvl = readInt("New reorder level (>=0): ");
    if (lvl < 0) {
        printf("Invalid level.\n");
        return;
    }

    reorder_setLevel(t, pid, lvl);
    if (reorder_saveCSV(csvPath, t) == 0) {
        printf("Saved reorder level: productId=%d level=%d -> %s\n", pid, lvl, csvPath);
    }
    else {
        printf("Failed to save %s\n", csvPath);
    }
}