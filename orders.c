#include "orders.h"
#include "pe_common.h"

// Resets the state of a node
void reset_node(node_t* node) {
    node_t new_node = {0};
    *node = new_node;
}

// Checks if two orders should be considered identical in the orderbook
bool compare_order(order_t order1, order_t order2) {
    return order1.is_buy == order2.is_buy && order1.product_id == order2.product_id && order1.price == order2.price;
}

// Checks if an order is valid
bool validate_order(order_t order, int product_count) {
    bool is_valid_id = order.product_id >= 0 && order.product_id < product_count;
    bool is_valid_price = order.price > 0 && order.price < MAX_PRICE;
    bool is_valid_quantity = order.quantity > 0 && order.quantity < MAX_QUANTITY;
    return is_valid_id && is_valid_price && is_valid_quantity;
}

// Checks if an order considered identical to itself in the orderbook exists
bool check_order_existence(order_t order, node_linked_list_t* order_list) {

    // Iterates through every node in the linked list until a match is found
    node_t* node = order_list->head;
    while (node != NULL) {
        if (compare_order(order, node->order) && node->order.quantity > 0) {
            return TRUE;
        }
        node = node->next;
    }

    return FALSE;
}

// Attempts to match a newly added order to an existing order
// Returns true if the order was successfully matched
bool match_order(node_t* added_node, node_t** matched_node, node_linked_list_t* order_list) {
    node_t* best_match = NULL;
    node_t* node = order_list->head;

    // Iterates through every node in the linked list
    while (node != NULL) {

        // Checks that the ids and quantities are valid
        if (node->order.product_id == added_node->order.product_id && node->order.quantity > 0) {

            if (added_node->order.is_buy && !node->order.is_buy && node->order.price <= added_node->order.price) {
                // The lowest sell order is the last match found in the linked list
                best_match = node;
            }
            if (!added_node->order.is_buy && node->order.is_buy && node->order.price >= added_node->order.price) {
                // The highest buy order is the first match found in the linked list
                best_match = node;
                break;
            }
        }
        node = node->next;
    }

    if (best_match != NULL) {
        *matched_node = best_match;
        return TRUE;
    }
    return FALSE;
}

// Adds an order to the linked list
void add_order(node_t** added_node, order_t new_order, node_linked_list_t* order_list) {
    // Finds a location to store the order
    int i = 0;
    for (i = 0; i < MAX_ORDERS; i++) {
        if (!order_list->order_array[i].is_valid) {
            order_list->order_array[i].order = new_order;
            break;
        }
    }

    if (i == MAX_ORDERS) {
        printf("This should never happen :(\n");
        return;
    }

    node_t* new_node = &order_list->order_array[i];
    new_node->is_valid = TRUE;
    node_t* node = order_list->head;
    node_t* prev_node = order_list->head;
    bool is_last = FALSE;
    bool is_first = FALSE;

    // Case where the order_list is empty
    if (node == NULL) {
        is_first = TRUE;
    }
    
    // Otherwise, the node is inserted in descending order
    else if (new_order.is_buy) {
        if (node->order.price < new_order.price) {
            is_first = TRUE;
        }
        while (node->order.price >= new_order.price) {
            if (node->next == NULL) {
                is_last = TRUE;
                break;
            }
            prev_node = node;
            node = node->next;
        }
    }

    else {
        if (node->order.price <= new_order.price) {
            is_first = TRUE;
        }
        while (node->order.price > new_order.price) {
            if (node->next == NULL) {
                is_last = TRUE;
                break;
            }
            prev_node = node;
            node = node->next;
        }
    }
    
    // If the node is last, there is no following node
    if (is_last) {
        node->next = new_node;
    }

    // If the node if first, there is no preceding node
    else if (is_first) {
        new_node->next = order_list->head;
        order_list->head = new_node;
    }
    else {
        new_node->next = node;
        prev_node->next = new_node;
    }
    
    *added_node = new_node;
}

// Cancels an order by removing it from the linked list
// Returns true if an order was successfully removed
bool cancel_order(node_linked_list_t* order_list, order_t* order, int trader_id, int order_id, int product_count) {
    bool order_found = FALSE;
    bool is_head = TRUE;
    node_t* node = order_list->head;
    node_t* prev_node = order_list->head;

    // Iterates through every node in the linked list until the matching order in found
    while (node != NULL) {
        if (node->order.trader_id == trader_id && node->order.order_id == order_id) {
            order_found = TRUE;
            break;
        }
        is_head = FALSE;
        prev_node = node;
        node = node->next;
    }

    // Returns false if the order was not found or the cancellation is illegal
    if (!order_found) {
        return FALSE;
    }
    if (!validate_order(node->order, product_count)) {
        return FALSE;
    }

    // If the node is at the head, there is no preceding node
    if (is_head) {
        *order = node->order;
        order_list->head = node->next;
        reset_node(node);
    }
    else if (!is_head) {
        *order = node->order;
        prev_node->next = node->next;
        reset_node(node);
    }

    return TRUE;
}

// Amends an order if it exists, and re-inserts it into the linked list
// Returns true if the order was successfully amended
bool amend_order(node_t** amended_node, node_linked_list_t* order_list, int trader_id, int order_id, int trader_count, int quantity, int price) {
    bool order_found = FALSE;
    bool is_head = TRUE;
    node_t* prev_node = order_list->head;
    node_t* node = order_list->head;

    // Iterates through every node in the linked list until the matching order in found
    while (node != NULL) {
        if (node->order.trader_id == trader_id && node->order.order_id == order_id) {
            order_found = TRUE;
            break;
        }
        is_head = FALSE;
        prev_node = node;
        node = node->next;
    }

    // Returns false if the order was not found or the node is illegal
    if (!order_found) {
        return FALSE;
    }
    order_t temp_order = node->order;
    if (!validate_order(temp_order, trader_count)) {
        return FALSE;
    }

    // Returns false if the ammendment is illegal
    temp_order.quantity = quantity;
    temp_order.price = price;
    if (!validate_order(temp_order, trader_count)) {
        return FALSE;
    }

    // Removes the node
    node->order = temp_order;
    if (is_head) {
        order_list->head = node->next;
    }
    else {
        prev_node->next = node->next;
    }

    // Re-inserts the node in the correct location
    add_order(amended_node, node->order, order_list);
    return TRUE;
}