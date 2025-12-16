#include "report.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ---------- helpers ---------- */

static int strcasestr_contains(const char* haystack, const char* needle) {
    if (!haystack || !needle) return 0;
    size_t nlen = strlen(needle);
    if (nlen == 0) return 1;

    for (const char* p = haystack; *p; ++p) {
        size_t i = 0;
        while (p[i] && i < nlen &&
            (char)tolower((unsigned char)p[i]) ==
            (char)tolower((unsigned char)needle[i])) {
            i++;
        }
        if (i == nlen) return 1;
    }
    return 0;
}

static int is_paid_line(const char* line) {
    return strcasestr_contains(line, "paid") || strcasestr_contains(line, "已支付");
}

/* 支持 "Total: 12.34" / "total=12.34" / "金额: 12.34" */
static int extract_total_amount(const char* line, double* out) {
    const char* keys[] = { "Total:", "total=", "TOTAL:", "总计:", "金额:" };
    for (size_t k = 0; k < sizeof(keys) / sizeof(keys[0]); ++k) {
        const char* pos = strstr(line, keys[k]);
        if (pos) {
            pos += strlen(keys[k]);
            while (*pos && isspace((unsigned char)*pos)) pos++;
            char* endp = NULL;
            double v = strtod(pos, &endp);
            if (endp != pos) {
                *out = v;
                return 1;
            }
        }
    }
    return 0;
}

/* 解析时间（可选）：
 * - epoch: createdAt=1700000000 / time=...
 * - ISO:   YYYY-MM-DD
 */
static int extract_time(const char* line, struct tm* out_tm) {
    const char* epochKeys[] = { "createdAt=", "time=", "timestamp=" };
    for (size_t k = 0; k < sizeof(epochKeys) / sizeof(epochKeys[0]); ++k) {
        const char* pos = strstr(line, epochKeys[k]);
        if (pos) {
            pos += strlen(epochKeys[k]);
            while (*pos && isspace((unsigned char)*pos)) pos++;
            char* endp = NULL;
            long long sec = strtoll(pos, &endp, 10);
            if (endp != pos && sec > 0) {
                time_t t = (time_t)sec;
                struct tm* lt = localtime(&t);
                if (lt) {
                    *out_tm = *lt;
                    return 1;
                }
            }
        }
    }

    /* ISO: find YYYY-MM-DD */
    const char* p = line;
    while (*p) {
        if (isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1]) &&
            isdigit((unsigned char)p[2]) && isdigit((unsigned char)p[3]) &&
            p[4] == '-' &&
            isdigit((unsigned char)p[5]) && isdigit((unsigned char)p[6]) &&
            p[7] == '-' &&
            isdigit((unsigned char)p[8]) && isdigit((unsigned char)p[9])) {

            int y = (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + (p[3] - '0');
            int m = (p[5] - '0') * 10 + (p[6] - '0');
            int d = (p[8] - '0') * 10 + (p[9] - '0');

            if (y >= 1970 && m >= 1 && m <= 12 && d >= 1 && d <= 31) {
                memset(out_tm, 0, sizeof(*out_tm));
                out_tm->tm_year = y - 1900;
                out_tm->tm_mon = m - 1;
                out_tm->tm_mday = d;
                return 1;
            }
        }
        ++p;
    }
    return 0;
}

/* 解析商品明细（可选）
 * 支持:
 * - "productId=3 qty=2"
 * - "ProductID: 3 Quantity: 2"
 */
typedef struct {
    int productId;
    long long qty;
} ProdAgg;

static int extract_product_qty(const char* line, int* pid, int* qty) {
    const char* pidKeys[] = { "productId=", "ProductID:", "pid=" };
    const char* qtyKeys[] = { "qty=", "Quantity:", "quantity=" };

    const char* ppos = NULL;
    for (size_t k = 0; k < sizeof(pidKeys) / sizeof(pidKeys[0]); ++k) {
        ppos = strstr(line, pidKeys[k]);
        if (ppos) { ppos += strlen(pidKeys[k]); break; }
    }
    if (!ppos) return 0;

    while (*ppos && isspace((unsigned char)*ppos)) ppos++;
    char* endp = NULL;
    long pval = strtol(ppos, &endp, 10);
    if (endp == ppos) return 0;

    const char* qpos = NULL;
    for (size_t k = 0; k < sizeof(qtyKeys) / sizeof(qtyKeys[0]); ++k) {
        qpos = strstr(line, qtyKeys[k]);
        if (qpos) { qpos += strlen(qtyKeys[k]); break; }
    }
    if (!qpos) return 0;

    while (*qpos && isspace((unsigned char)*qpos)) qpos++;
    endp = NULL;
    long qval = strtol(qpos, &endp, 10);
    if (endp == qpos) return 0;

    *pid = (int)pval;
    *qty = (int)qval;
    return 1;
}

static void agg_add(ProdAgg** arr, size_t* n, size_t* cap, int pid, int qty) {
    for (size_t i = 0; i < *n; ++i) {
        if ((*arr)[i].productId == pid) {
            (*arr)[i].qty += qty;
            return;
        }
    }
    if (*n >= *cap) {
        size_t newCap = (*cap == 0) ? 16 : (*cap * 2);
        ProdAgg* nd = (ProdAgg*)realloc(*arr, newCap * sizeof(ProdAgg));
        if (!nd) return;
        *arr = nd;
        *cap = newCap;
    }
    (*arr)[*n].productId = pid;
    (*arr)[*n].qty = qty;
    (*n)++;
}

static int cmp_qty_desc(const void* a, const void* b) {
    const ProdAgg* pa = (const ProdAgg*)a;
    const ProdAgg* pb = (const ProdAgg*)b;
    if (pa->qty < pb->qty) return 1;
    if (pa->qty > pb->qty) return -1;
    return 0;
}

/* products.csv 简单读取：假设前两列为 id,name */
typedef struct {
    int id;
    char name[64];
} ProductName;

static int load_product_names(const char* productsCsv, ProductName** out, size_t* outN) {
    FILE* fp = fopen(productsCsv, "r");
    if (!fp) return -1;

    ProductName* arr = NULL;
    size_t n = 0, cap = 0;
    char line[512];

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;

        char* p = line;
        char* comma = strchr(p, ',');
        if (!comma) continue;
        *comma = '\0';
        int id = atoi(p);

        p = comma + 1;
        comma = strchr(p, ',');
        if (comma) *comma = '\0';

        size_t len = strlen(p);
        while (len && (p[len - 1] == '\n' || p[len - 1] == '\r')) p[--len] = '\0';

        if (n >= cap) {
            size_t newCap = (cap == 0) ? 16 : cap * 2;
            ProductName* nd = (ProductName*)realloc(arr, newCap * sizeof(ProductName));
            if (!nd) break;
            arr = nd;
            cap = newCap;
        }
        arr[n].id = id;
        strncpy_s(arr[n].name, sizeof(arr[n].name), p, sizeof(arr[n].name) - 1);
        arr[n].name[sizeof(arr[n].name) - 1] = '\0';
        n++;
    }
    fclose(fp);

    *out = arr;
    *outN = n;
    return 0;
}

static const char* find_product_name(const ProductName* arr, size_t n, int id) {
    for (size_t i = 0; i < n; ++i) {
        if (arr[i].id == id) return arr[i].name;
    }
    return NULL;
}

/* ---------- public APIs ---------- */

void report_showMenu(void) {
    printf("\n[Reports]\n");
    printf("13. Sales summary (from orders.log)\n");
    printf("14. Monthly sales (from orders.log)\n");
    printf("15. Top products (from orders.log + products.csv)\n");
}

void report_salesSummaryFromLog(const char* orderLogPath) {
    FILE* fp = fopen(orderLogPath, "r");
    if (!fp) {
        printf("Cannot open %s\n", orderLogPath);
        return;
    }

    long long lines = 0;
    long long paidLines = 0;
    double totalPaid = 0.0;

    char buf[1024];
    while (fgets(buf, sizeof(buf), fp)) {
        lines++;
        if (is_paid_line(buf)) {
            paidLines++;
            double amt = 0.0;
            if (extract_total_amount(buf, &amt)) totalPaid += amt;
        }
    }
    fclose(fp);

    printf("\n=== Sales Summary (from %s) ===\n", orderLogPath);
    printf("Log lines: %lld\n", lines);
    printf("Paid records: %lld\n", paidLines);
    printf("Total revenue (paid): %.2f\n", totalPaid);
    if (paidLines > 0) {
        printf("Average per paid record: %.2f\n", totalPaid / (double)paidLines);
    }
}

typedef struct {
    int year;
    int month;
    long long paidCount;
    double paidSum;
} MonthAgg;

static void monthagg_add(MonthAgg** arr, size_t* n, size_t* cap,
    int y, int m, double amt) {
    for (size_t i = 0; i < *n; ++i) {
        if ((*arr)[i].year == y && (*arr)[i].month == m) {
            (*arr)[i].paidCount++;
            (*arr)[i].paidSum += amt;
            return;
        }
    }
    if (*n >= *cap) {
        size_t newCap = (*cap == 0) ? 12 : (*cap * 2);
        MonthAgg* nd = (MonthAgg*)realloc(*arr, newCap * sizeof(MonthAgg));
        if (!nd) return;
        *arr = nd;
        *cap = newCap;
    }
    (*arr)[*n].year = y;
    (*arr)[*n].month = m;
    (*arr)[*n].paidCount = 1;
    (*arr)[*n].paidSum = amt;
    (*n)++;
}

static int cmp_month_asc(const void* a, const void* b) {
    const MonthAgg* ma = (const MonthAgg*)a;
    const MonthAgg* mb = (const MonthAgg*)b;
    int ka = ma->year * 100 + ma->month;
    int kb = mb->year * 100 + mb->month;
    return (ka > kb) - (ka < kb);
}

void report_monthlySalesFromLog(const char* orderLogPath) {
    FILE* fp = fopen(orderLogPath, "r");
    if (!fp) {
        printf("Cannot open %s\n", orderLogPath);
        return;
    }

    MonthAgg* months = NULL;
    size_t n = 0, cap = 0;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (!is_paid_line(line)) continue;

        double amt = 0.0;
        if (!extract_total_amount(line, &amt)) continue;

        struct tm t;
        if (!extract_time(line, &t)) {
            time_t now = time(NULL);
            struct tm* lt = localtime(&now);
            if (!lt) continue;
            t = *lt;
        }
        monthagg_add(&months, &n, &cap, t.tm_year + 1900, t.tm_mon + 1, amt);
    }
    fclose(fp);

    qsort(months, n, sizeof(MonthAgg), cmp_month_asc);

    printf("\n=== Monthly Sales (paid only) ===\n");
    printf("%-7s %-10s %-10s\n", "Month", "PaidCount", "Revenue");
    for (size_t i = 0; i < n; ++i) {
        printf("%04d-%02d %-10lld %-10.2f\n",
            months[i].year, months[i].month, months[i].paidCount, months[i].paidSum);
    }
    free(months);
}

void report_topProductsFromLog(const char* orderLogPath,
    const char* productsCsvPath,
    int topN) {
    FILE* fp = fopen(orderLogPath, "r");
    if (!fp) {
        printf("Cannot open %s\n", orderLogPath);
        return;
    }

    ProdAgg* aggs = NULL;
    size_t n = 0, cap = 0;

    char line[1024];
    int parsedAny = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!is_paid_line(line)) continue;

        int pid = 0, qty = 0;
        if (extract_product_qty(line, &pid, &qty)) {
            parsedAny = 1;
            agg_add(&aggs, &n, &cap, pid, qty);
        }
    }
    fclose(fp);

    if (!parsedAny) {
        printf("\nTop products: cannot parse productId/qty from %s.\n", orderLogPath);
        printf("Need item lines like: productId=3 qty=2 (and include paid marker).\n");
        free(aggs);
        return;
    }

    qsort(aggs, n, sizeof(ProdAgg), cmp_qty_desc);

    ProductName* names = NULL;
    size_t namesN = 0;
    if (load_product_names(productsCsvPath, &names, &namesN) != 0) {
        printf("Warning: cannot open %s, will show productId only.\n", productsCsvPath);
    }

    if (topN <= 0) topN = 10;
    if ((size_t)topN > n) topN = (int)n;

    printf("\n=== Top Products (paid only) ===\n");
    printf("%-6s %-20s %-10s\n", "ID", "Name", "Qty");
    for (int i = 0; i < topN; ++i) {
        const char* nm = names ? find_product_name(names, namesN, aggs[i].productId) : NULL;
        printf("%-6d %-20s %-10lld\n",
            aggs[i].productId,
            (nm ? nm : "(unknown)"),
            aggs[i].qty);
    }

    free(names);
    free(aggs);
}