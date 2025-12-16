#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "product.h"
#include "inventory.h"
#include "order.h"
#include "persistence.h"
#include "user.h"
#include "utils.h"

#define PRODUCT_FILE "products.csv"
#define ORDER_FILE   "orders.log"
#define USER_FILE    "users.csv"

/* -------- In-memory order list management -------- */
typedef struct {
    Order* data;
    size_t  size;
    size_t  capacity;
} OrderList;

static void initOrderList(OrderList* olist) {
    olist->data = NULL;
    olist->size = 0;
    olist->capacity = 0;
}

static void freeOrderList(OrderList* olist) {
    if (!olist) return;
    for (size_t i = 0; i < olist->size; ++i) {
        freeOrder(&olist->data[i]);
    }
    free(olist->data);
    olist->data = NULL;
    olist->size = 0;
    olist->capacity = 0;
}

static void ensureOrderCapacity(OrderList* olist) {
    if (olist->size >= olist->capacity) {
        size_t newCap = (olist->capacity == 0) ? 4 : olist->capacity * 2;
        Order* newData = (Order*)realloc(olist->data, newCap * sizeof(Order));
        if (!newData) {
            fprintf(stderr, "Order list expansion failed\n");
            exit(EXIT_FAILURE);
        }
        olist->data = newData;
        olist->capacity = newCap;
    }
}

static Order* addOrderToList(OrderList* olist, int orderId) {
    ensureOrderCapacity(olist);
    Order* o = &olist->data[olist->size++];
    initOrder(o, orderId);
    return o;
}

static Order* findOrderById(OrderList* olist, int id) {
    for (size_t i = 0; i < olist->size; ++i) {
        if (olist->data[i].orderId == id) return &olist->data[i];
    }
    return NULL;
}

static void listAllOrders(const OrderList* olist) {
    if (olist->size == 0) {
        printf("No orders found.\n");
        return;
    }
    printf("=== Order List ===\n");
    printf("%-6s %-10s %-12s %-10s\n", "ID", "Status", "ItemCount", "Total");
    for (size_t i = 0; i < olist->size; ++i) {
        const Order* o = &olist->data[i];
        printf("%-6d %-10s %-12zu %-10.2f\n",
            o->orderId,
            orderStatusToStr(o->status),
            o->size,
            o->totalAmount);
    }
}

/* -------- Globals -------- */
static int nextOrderId = 1;
static ProductList products;
static OrderList   orders;
static UserList    users;
static User* currentUser = NULL;

/* -------- Auth check -------- */
static int requireLogin() {
    if (!currentUser) {
        printf("Please login first.\n");
        return 0;
    }
    return 1;
}

/* -------- Menu -------- */
static void menu() {
    printf("\n===== 简单销售系统 (v2) =====\n");
    printf("[Product]\n");
    printf("1. List products\n");
    printf("2. Add product (login required)\n");
    printf("3. Modify product (login required)\n");
    printf("4. Delete product (login required)\n");
    printf("[Order]\n");
    printf("5. Create order (login required)\n");
    printf("6. List orders\n");
    printf("7. Pay order (login required)\n");
    printf("8. Cancel order (login required)\n");
    printf("[Files]\n");
    printf("9. Save products to file\n");
    printf("[User]\n");
    printf("10. Register\n");
    printf("11. Login\n");
    printf("12. Logout\n");
    printf("0. Exit\n");
}

/* -------- User handlers -------- */
static void handleRegister() {
    char uname[32];
    char pwd[64];
    readLine("New username: ", uname, sizeof(uname));
    readLine("Password: ", pwd, sizeof(pwd));
    if (!*uname || !*pwd) {
        printf("Username or password empty. Registration failed.\n");
        return;
    }
    if (findUserByName(&users, uname)) {
        printf("Username already exists.\n");
        return;
    }
    User* u = registerUser(&users, uname, pwd);
    if (u) {
        saveUsersToCSV(USER_FILE, &users);
        printf("Registration successful. UserID=%d\n", u->id);
    }
    else {
        printf("Registration failed.\n");
    }
}

static void handleLogin() {
    if (currentUser) {
        printf("Already logged in as: %s. Logout first to switch user.\n", currentUser->username);
        return;
    }
    char uname[32];
    char pwd[64];
    readLine("Username: ", uname, sizeof(uname));
    readLine("Password: ", pwd, sizeof(pwd));
    User* u = authenticate(&users, uname, pwd);
    if (u) {
        currentUser = u;
        printf("登陆成功，欢迎%s\n", currentUser->username);
        return;
    }
    printf("Login failed: user not found or wrong password.\n");
}

static void handleLogout() {
    if (!currentUser) {
        printf("No user is logged in.\n");
        return;
    }
    printf("User %s logged out.\n", currentUser->username);
    currentUser = NULL;
}

/* -------- Product handlers -------- */
static void handleAddProduct() {
    if (!requireLogin()) return;
    char name[64];
    readLine("Product name: ", name, sizeof(name));
    double price = readDouble("Product price: ");
    int stock = readInt("Initial stock: ");
    int id = addProduct(&products, name, price, stock);
    if (id > 0) {
        printf("Added. ID=%d\n", id);
    }
    else {
        printf("Add failed.\n");
    }
}

static void handleModifyProduct() {
    if (!requireLogin()) return;
    int id = readInt("Product ID to modify: ");
    Product* p = findProductById(&products, id);
    if (!p) {
        printf("Product not found.\n");
        return;
    }
    printf("Current: Name=%s Price=%.2f Stock=%d\n", p->name, p->price, p->stock);
    char name[64];
    readLine("New name (leave empty to keep): ", name, sizeof(name));
    double price;
    char buf[64];
    printf("New price (negative to keep): ");
    if (!fgets(buf, sizeof(buf), stdin)) {
        printf("Input error.\n");
        return;
    }
    price = atof(buf);
    int stock;
    printf("New stock (negative to keep): ");
    if (!fgets(buf, sizeof(buf), stdin)) {
        printf("Input error.\n");
        return;
    }
    stock = atoi(buf);
    if (modifyProduct(&products, id,
        (*name ? name : NULL),
        price,
        stock) == 0) {
        printf("Modify success.\n");
    }
    else {
        printf("Modify failed.\n");
    }
}

static int productUsedInActiveOrders(int productId) {
    for (size_t i = 0; i < orders.size; ++i) {
        Order* o = &orders.data[i];
        if (o->status == ORDER_CANCELLED) continue;
        for (size_t k = 0; k < o->size; ++k) {
            if (o->items[k].productId == productId) {
                return 1;
            }
        }
    }
    return 0;
}

static void handleDeleteProduct() {
    if (!requireLogin()) return;
    int id = readInt("Product ID to delete: ");
    Product* p = findProductById(&products, id);
    if (!p) {
        printf("Product not found.\n");
        return;
    }
    if (productUsedInActiveOrders(id)) {
        printf("Product appears in active orders. Deletion denied.\n");
        return;
    }
    if (deleteProduct(&products, id) == 0) {
        printf("Delete success.\n");
    }
    else {
        printf("Delete failed.\n");
    }
}

/* -------- Order handlers -------- */
static void restoreStockOnCancel(Order* order) {
    for (size_t i = 0; i < order->size; ++i) {
        OrderItem* it = &order->items[i];
        Product* p = findProductById(&products, it->productId);
        if (p) {
            increaseStock(p, it->quantity);
        }
    }
}

static void handleCreateOrder() {
    if (!requireLogin()) return;
    Order* o = addOrderToList(&orders, nextOrderId++);
    printf("Creating new order. OrderID=%d\n", o->orderId);
    while (1) {
        int pid = readInt("Enter product ID (0 to finish): ");
        if (pid == 0) break;
        Product* p = findProductById(&products, pid);
        if (!p) {
            printf("Product not found.\n");
            continue;
        }
        printf("Product: %s Price: %.2f Stock: %d\n", p->name, p->price, p->stock);
        int qty = readInt("Quantity: ");
        if (qty <= 0) {
            printf("Invalid quantity.\n");
            continue;
        }
        if (deductStock(p, qty) != 0) {
            printf("Insufficient stock.\n");
            continue;
        }
        addOrderItem(o, p, qty);
        printf("Added to order.\n");
    }
    if (o->size == 0) {
        printf("Empty order. Auto-cancel.\n");
        cancelOrder(o);
        restoreStockOnCancel(o);
    }
    printOrder(o);
    appendOrderToFile(ORDER_FILE, o);
}

static void handleListOrders() {
    listAllOrders(&orders);
    int detailId = readInt("Enter order ID to view details (0 to skip): ");
    if (detailId != 0) {
        Order* o = findOrderById(&orders, detailId);
        if (!o) {
            printf("Order not found.\n");
            return;
        }
        printOrder(o);
    }
}

static void handlePayOrder() {
    if (!requireLogin()) return;
    int id = readInt("Order ID to pay: ");
    Order* o = findOrderById(&orders, id);
    if (!o) {
        printf("Order not found.\n");
        return;
    }
    if (o->status != ORDER_CREATED) {
        printf("Order is %s, cannot pay.\n", orderStatusToStr(o->status));
        return;
    }
    markOrderPaid(o);
    printOrder(o);
    appendOrderToFile(ORDER_FILE, o);
    printf("Payment simulated.\n");
}

static void handleCancelOrder() {
    if (!requireLogin()) return;
    int id = readInt("Order ID to cancel: ");
    Order* o = findOrderById(&orders, id);
    if (!o) {
        printf("Order not found.\n");
        return;
    }
    if (o->status != ORDER_CREATED) {
        printf("Order is %s, cannot cancel.\n", orderStatusToStr(o->status));
        return;
    }
    cancelOrder(o);
    restoreStockOnCancel(o);
    printOrder(o);
    appendOrderToFile(ORDER_FILE, o);
    printf("Order cancelled and stock restored.\n");
}

/* -------- File save handler -------- */
static void handleSaveProducts() {
    if (saveProductsToCSV(PRODUCT_FILE, &products) == 0) {
        printf("Products saved -> %s\n", PRODUCT_FILE);
    }
    else {
        printf("Save products failed.\n");
    }
}

/* -------- Main -------- */
int main() {
    initProductList(&products);
    initOrderList(&orders);
    initUserList(&users);

    int loadedProd = loadProductsFromCSV(PRODUCT_FILE, &products);
    if (loadedProd >= 0) {
        printf("Loaded %d products.\n", loadedProd);
    }
    else {
        printf("Product file not found. Starting with empty list.\n");
    }
    int loadedUsers = loadUsersFromCSV(USER_FILE, &users);
    if (loadedUsers >= 0) {
        printf("Loaded %d users.\n", loadedUsers);
    }
    else {
        printf("User file not found. Starting with empty user list.\n");
    }

    int choice;
    while (1) {
        menu();
        choice = readInt("Select: ");
        switch (choice) {
        case 1:
            listProducts(&products);
            break;
        case 2:
            handleAddProduct();
            break;
        case 3:
            handleModifyProduct();
            break;
        case 4:
            handleDeleteProduct();
            break;
        case 5:
            handleCreateOrder();
            break;
        case 6:
            handleListOrders();
            break;
        case 7:
            handlePayOrder();
            break;
        case 8:
            handleCancelOrder();
            break;
        case 9:
            handleSaveProducts();
            break;
        case 10:
            handleRegister();
            break;
        case 11:
            handleLogin();
            break;
        case 12:
            handleLogout();
            break;
        case 0:
            goto EXIT;
        default:
            printf("Invalid option.\n");
        }
    }

EXIT:
    if (saveProductsToCSV(PRODUCT_FILE, &products) == 0)
        printf("Products saved on exit.\n");
    if (saveUsersToCSV(USER_FILE, &users) == 0)
        printf("Users saved on exit.\n");

    freeProductList(&products);
    freeOrderList(&orders);
    freeUserList(&users);
    return 0;
}