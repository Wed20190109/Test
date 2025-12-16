#pragma once
#pragma once
#ifndef REPORT_H
#define REPORT_H

/* 从 orders.log 生成报表（纯文本解析，尽量容错）
 * - 仅统计“已支付”订单（默认关键字：PAID / paid / 已支付 / Paid）
 */

void report_showMenu(void);

/* 总览：订单数、已支付订单数、销售总额、客单价 */
void report_salesSummaryFromLog(const char* orderLogPath);

/* 按月份汇总（YYYY-MM）：订单数、已支付订单数、销售额 */
void report_monthlySalesFromLog(const char* orderLogPath);

/* 热销商品 TopN（需要你有 products.csv：productId,name,...）
 * 从日志里解析 productId 与 quantity（如果日志里有）
 * 若日志里没有商品明细，这个函数会提示“无法统计”。
 */
void report_topProductsFromLog(const char* orderLogPath,
    const char* productsCsvPath,
    int topN);

#endif