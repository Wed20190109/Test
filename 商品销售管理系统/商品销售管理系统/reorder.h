#pragma once
#pragma once
#ifndef REORDER_H
#define REORDER_H

#include <stddef.h>
#include "product.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* reorder_levels.csv schema: productId,reorderLevel */

    typedef struct {
        int productId;
        int reorderLevel;
    } ReorderLevel;

    typedef struct {
        ReorderLevel* data;
        size_t size;
        size_t capacity;
    } ReorderTable;

    void reorder_init(ReorderTable* t);
    void reorder_free(ReorderTable* t);

    int  reorder_loadCSV(const char* path, ReorderTable* t);
    int  reorder_saveCSV(const char* path, const ReorderTable* t);

    /* 获取/设置阈值 */
    int  reorder_getLevel(const ReorderTable* t, int productId, int defaultLevel);
    void reorder_setLevel(ReorderTable* t, int productId, int level);
    void reorder_remove(ReorderTable* t, int productId);

    /* 报表/列表 */
    void reorder_printLowStock(const ProductList* products,
        const ReorderTable* t,
        int defaultLevel);

    void reorder_printReplenishList(const ProductList* products,
        const ReorderTable* t,
        int defaultLevel);

    /* 交互：设置某商品阈值并保存（由 main.c 调用） */
    void reorder_interactiveSetLevel(const char* csvPath,
        ReorderTable* t,
        ProductList* products,
        int defaultLevel);

#ifdef __cplusplus
}
#endif

#endif