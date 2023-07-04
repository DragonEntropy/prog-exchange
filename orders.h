#ifndef ORDERS
#define ORDERS

#include "pe_common.h"

#define MAX_ORDERS (500)

// Stores data related to a single product type
typedef struct product_t {
	int id;
	char name[BUFFER_SIZE];
	int buy_levels;
	int sell_levels;
} product_t;

// Stores data related to an instance of an order
typedef struct order_t {
	int trader_id;
	bool is_buy;
	int order_id;
	int product_id;
	char product[BUFFER_SIZE];
	long price;
	int quantity;
} order_t;

// A node for a linked list
typedef struct node_t {
	bool is_valid;
	order_t order;
	struct node_t* next;
} node_t;

// A linked list data structure for order nodes
typedef struct node_linked_list_t {
	node_t* head;
	node_t* order_array;
} node_linked_list_t;

void reset_node(node_t* node);
bool compare_order(order_t order1, order_t order2);
bool validate_order(order_t order, int product_count);
bool check_order_existence(order_t order, node_linked_list_t* order_list);
bool match_order(node_t* added_node, node_t** matched_node, node_linked_list_t* order_list);
void add_order(node_t** added_node, order_t new_order, node_linked_list_t* order_list);
bool amend_order(node_t** amended_node, node_linked_list_t* order_list, int trader_id, int order_id, int trader_count, int quantity, int price);
bool cancel_order(node_linked_list_t* order_list, order_t* order, int trader_id, int order_id, int product_count);

#endif