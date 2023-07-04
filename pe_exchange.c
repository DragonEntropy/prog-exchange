/*
 * comp2017 - assignment 3
 * Alexander Jephtha
 * 520446056
 */

#include "orders.h"
#include "pe_exchange.h"
#include "pe_common.h"

// Global variables are required for signal handlers
// signal_data is the signal queue
signal_data_t signal_data = {0};
int trader_count;

// Prints a string using PEX formating
void pex_print(char* output) {
    printf("%s%s", LOG_PREFIX, output);
}

// Formats an order for the orderbook
void format_order(char* output, order_t* order, int quantity, int order_count) {
    char* type = order->is_buy ? "BUY" : "SELL";
    if (order_count == 1) {
        sprintf(output, "\t\t%s %d @ $%ld (1 order)\n", type, quantity, order->price);
    }
    else {
        sprintf(output, "\t\t%s %d @ $%ld (%d orders)\n", type, quantity, order->price, order_count);
    }
}

// Formats a trader for the trader positions summary
void format_position(char* output, trader_data_t trader, exchange_data_t* data, int product_id, bool is_last) {
    char suffix = is_last ? '\n' : ',';
    sprintf(output, " %s %d ($%ld)%c", data->products[product_id].name, trader.product_counts[product_id], trader.product_costs[product_id], suffix);
}

// Prints the orderbook
void print_orderbook(exchange_data_t* data) {
    pex_print("\t--ORDERBOOK--\n");
    char output[OUTPUT_BUFFER_SIZE];

    // Iterates through each valid product
    for (int p = 0; p < data->product_count; p++) {
        product_t product = data->products[p];
        sprintf(output, "\tProduct: %s; Buy levels: %d; Sell levels: %d\n", product.name, product.buy_levels, product.sell_levels);
        pex_print(output);

        // Prints every product by iterating through the linked list
        node_t* node = data->order_list->head;
        while (node != NULL) {

            // Checks that the order corresponds to the product
            if (node->order.product_id == product.id && node->order.quantity > 0) {
                int count = 1;
                order_t reference_order = node->order;
                int quantity = reference_order.quantity;
                node = node->next;

                // Tries to group equivalent orders into one line
                while (node != NULL) {

                    // Check if two orders are equivalent
                    if (compare_order(node->order, reference_order)) {
                        count++;
                        quantity += node->order.quantity;
                    }

                    // Breaks when the price descends below the equivalent price
                    else if (node->order.price != reference_order.price) {
                        break;
                    }

                    node = node->next;
                }

                // Prints the order line
                format_order(output, &reference_order, quantity, count);
                pex_print(output);
            }

            // Otherwise moves to the next product
            else {
                node = node->next;
            }
        }
    }
}

// Prints the positions of each trader
void print_positions(exchange_data_t* data) {
    pex_print("\t--POSITIONS--\n");
    char output[OUTPUT_BUFFER_SIZE];

    // Iterates through each trader
    for (int t = 0; t < data->trader_count; t++) {
        sprintf(output, "\tTrader %d:", t);
        pex_print(output);

        // Prints the status of the trader for each product
        for (int p = 0; p < data->product_count; p++) {
            format_position(output, data->traders[t], data, p, p == data->product_count - 1);
            printf("%s", output);
        }
    }
}

// Handles the SIGUSR1 signal from traders
void handle_sigusr1(int sig, siginfo_t* info, void* context) {
    signal_t signal = {
        .trader_fd = (int)info->si_pid,
        .signal_type = 1
    };
    signal_data.signals[signal_data.write_index] = signal;
    signal_data.write_index = (signal_data.write_index + 1) % trader_count;
    signal_data.signal_count++;
}

// Handles the SIGCHLD signal from traders when they terminate
void handle_sigchld(int sig, siginfo_t* info, void* context) {
    signal_t signal = {
        .trader_fd = (int)info->si_pid,
        .signal_type = 2
    };
    signal_data.signals[signal_data.write_index] = signal;
    signal_data.write_index = (signal_data.write_index + 1) % trader_count;
    signal_data.signal_count++;
}

// Finds the id of a product from its name
int find_product_id(product_t* products, int product_count, char* name) {
    for (int p = 0; p < product_count; p++) {
        if (strcmp(products[p].name, name) == 0) {
            return p;
        }
    }
    return -1;
}

// Finds the id of a trader from its file descriptor
int find_trader_id(int* trader_fds, int trader_count, int trader_fd) {
    for (int t = 0; t < trader_count; t++) {
        if (trader_fds[t] == trader_fd) {
            return t;
        }
    }
    return -1;
}

// Replaces the semicolon with a null byte in a command
// Returns true if the command terminated with a semicolon
bool clean_command(char* command) {
    while(*command != ';' && *command != '\0') {
        command++;
    }

    bool has_semicolon = (*command == ';');
    *command = '\0';
    return has_semicolon;
}

// Sends an invalid message to a trader
void send_invalid_signal(int sender_id, exchange_data_t* data) {
    char* buffer = "INVALID;";
    write(data->exchange_pipes[sender_id], buffer, strlen(buffer));
    kill(data->exchange_pipes[sender_id], SIGUSR1);
}

// Sends a market message one and/or all traders
void send_market_signals(order_t order, int sender_id, message_type_t message, exchange_data_t* data) {
    char buffer[OUTPUT_BUFFER_SIZE];
    switch (message) {

        // When an order is accepted successfully
        case ACCEPTED:
            sprintf(buffer, "ACCEPTED %d;", order.order_id);
            break;

        // When an order is amended successfully
        case AMENDED:
            sprintf(buffer, "AMENDED %d;", order.order_id);
            break;

        // When an order is cancelled successfully
        case CANCELLED:
            sprintf(buffer, "CANCELLED %d;", order.order_id);
            break;

        // When a request is invalid
        default:
            send_invalid_signal(sender_id, data);
            return;
    }

    // Writes the message and sends the signal to the sending trader
    write(data->exchange_pipes[sender_id], buffer, strlen(buffer));
    kill(data->exchange_pipes[sender_id], SIGUSR1);

    char* type = order.is_buy ? "BUY" : "SELL";

    // Writes an sends a market message to all other traders
    if (message == CANCELLED) {
        sprintf(buffer, "MARKET %s %s 0 0;", type, order.product);
    }
    else {
        sprintf(buffer, "MARKET %s %s %d %ld;", type, order.product, order.quantity, order.price);
    }

    for (int trader_id = 0; trader_id < trader_count; trader_id++) {
        if (trader_id != sender_id && data->trader_status[trader_id]) {
            write(data->exchange_pipes[trader_id], buffer, strlen(buffer));
            kill(data->exchange_pipes[trader_id], SIGUSR1);
        }
    }
}

// Sends a fill message to each trader if they are active
void send_fill_signals(order_t buyer_order, order_t seller_order, int buyer_id, int seller_id, int quantity, exchange_data_t* data) {
    char buffer[OUTPUT_BUFFER_SIZE];
    if (data->trader_status[buyer_order.trader_id]) {
        sprintf(buffer, "FILL %d %d;", buyer_order.order_id, quantity);
        write(data->exchange_pipes[buyer_id], buffer, strlen(buffer));
        kill(data->exchange_pipes[buyer_id], SIGUSR1);
    }
    if (data->trader_status[seller_order.trader_id]) {
        sprintf(buffer, "FILL %d %d;", seller_order.order_id, quantity);
        write(data->exchange_pipes[seller_id], buffer, strlen(buffer));
        kill(data->exchange_pipes[seller_id], SIGUSR1);
    }
}

// Updates the buy and sell levels for each product for the orderbook
void update_levels(order_t order, int sign, exchange_data_t* data) {
    product_t* product = &data->products[order.product_id];

    // Performs this by checking for the existence of the same order within the linked list
    if (!check_order_existence(order, data->order_list)) {
        if (order.is_buy) {
            product->buy_levels += sign;
        }
        else {
            product->sell_levels += sign;
        }
    }
}

// Processes a matched order
void process_match(order_t* added_order, order_t* matched_order, exchange_data_t* data) {
    order_t* buy_order;
    order_t* sell_order;
    bool buyer_takes_fee = FALSE;

    // Determines which trader takes the fee
    if (added_order->is_buy) {
        buy_order = added_order;
        sell_order = matched_order;
        buyer_takes_fee = TRUE;
    }
    else {
        buy_order = matched_order;
        sell_order = added_order;
    }

    trader_data_t* buy_trader = &data->traders[buy_order->trader_id];
    trader_data_t* sell_trader = &data->traders[sell_order->trader_id];

    // Determines the quantity to be traded
    int quantity_traded = fmin(buy_order->quantity, sell_order->quantity);
    buy_order->quantity -= quantity_traded;
    sell_order->quantity -= quantity_traded;

    // Calculates the trade price and the fees
    int matched_price = matched_order->price;
    long value = (long)matched_price * (long)quantity_traded;
    long fee = round(value * (double)FEE_PERCENTAGE / 100);
    long buy_fee = 0;
    long sell_fee = 0;
    if (buyer_takes_fee) {
        buy_fee = fee;
    }
    else {
        sell_fee = fee;
    }

    // Updates the traders' balances and product counts
    int product_id = buy_order->product_id;
    buy_trader->product_counts[product_id] += quantity_traded;
    sell_trader->product_counts[product_id] -= quantity_traded;
    buy_trader->product_costs[product_id] -= value + buy_fee;
    sell_trader->product_costs[product_id] += value - sell_fee;

    // Saves the 1% fee to the exchange
    data->fees += fee;

    char output[OUTPUT_BUFFER_SIZE];
    sprintf(output, " Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", matched_order->order_id, matched_order->trader_id, added_order->order_id, added_order->trader_id, value, fee);
    pex_print(output);

    // Sends fill signals to both traders
    if (added_order->is_buy) {
        send_fill_signals(*added_order, *matched_order, added_order->trader_id, matched_order->trader_id, quantity_traded, data);
    }
    else {
        send_fill_signals(*matched_order, *added_order, matched_order->trader_id, added_order->trader_id, quantity_traded, data);
    }
}

// BUY/SELL <ID> <PRODUCT> <QTY> <PRICE>;
// Handles the buy and sell messages
// Returns false if the order is invalid
bool buy_sell(char* command, int* tokens, bool is_buy, int trader_id, exchange_data_t* data) {
    char id_string[BUFFER_SIZE];
    char product_string[BUFFER_SIZE];
    char quantity_string[BUFFER_SIZE];
    char price_string[BUFFER_SIZE];

    get_token(command, tokens, 1, id_string);
    get_token(command, tokens, 2, product_string);
    get_token(command, tokens, 3, quantity_string);
    get_token(command, tokens, 4, price_string);

    if (!is_number(id_string) || is_number(product_string) || !is_number(quantity_string) || !is_number(price_string)) {
        send_invalid_signal(trader_id, data);
        return FALSE;
    }

    int id = atoi(id_string);
    int quantity = atoi(quantity_string);
    int price = atoi(price_string);

    order_t order = {
        .trader_id = trader_id,
        .is_buy = is_buy,
        .order_id = id,
        .product_id = find_product_id(data->products, data->product_count, product_string),
        .price = price,
        .quantity = quantity
    };
    strcpy(order.product, product_string);

    // Checks that the order is legal
    if (!validate_order(order, data->product_count) || data->traders[trader_id].next_order_id != id) {
        send_market_signals(order, trader_id, INVALID, data);
        return FALSE;
    }

    node_t* added_node = NULL;

    // Updates levels and order ids
    update_levels(order, 1, data);
    data->traders[trader_id].next_order_id++;

    // Adds the order to the market and sends signals to each trader
    add_order(&added_node, order, data->order_list);
    send_market_signals(order, trader_id, ACCEPTED, data);

    // Checks for matches
    bool is_matched = TRUE;
    node_t* matched_node = NULL;
    order_t* added_order = &added_node->order;

    // While there is a match available, it will process it
    while (is_matched && added_order->quantity > 0) {
        is_matched = match_order(added_node, &matched_node, data->order_list);
        order_t* matched_order = &matched_node->order;

        if (is_matched) {
            process_match(added_order, matched_order, data);
            update_levels(*added_order, -1, data);
            update_levels(*matched_order, -1, data);
        }
    }

    return TRUE;
}

// AMEND <ID> <QTY> <PRICE>;
// Handles the amend message
// Returns false if the order is invalid
bool amend(char* command, int* tokens, int trader_id, exchange_data_t* data) {
    char id_string[BUFFER_SIZE];
    char quantity_string[BUFFER_SIZE];
    char price_string[BUFFER_SIZE];

    get_token(command, tokens, 1, id_string);
    get_token(command, tokens, 2, quantity_string);
    get_token(command, tokens, 3, price_string);

    if (!is_number(id_string) || !is_number(quantity_string) || !is_number(price_string)) {
        send_invalid_signal(trader_id, data);
        return FALSE;
    }

    int id = atoi(id_string);
    int quantity = atoi(quantity_string);
    int price = atoi(price_string);

    // Checks if the updated order is legal
    node_t* amended_node = NULL;
    bool is_valid = amend_order(&amended_node, data->order_list, trader_id, id, data->trader_count, quantity, price);
    if (!is_valid) {
        send_invalid_signal(trader_id, data);
        return FALSE;
    }

    // Sends signals to each trader
    send_market_signals(amended_node->order, trader_id, AMENDED, data);

    bool is_matched = TRUE;
    node_t* matched_node = NULL;
    order_t* amended_order = &amended_node->order;

    // While there is a match available, it will process it
    while (is_matched && amended_order->quantity > 0) {
        is_matched = match_order(amended_node, &matched_node, data->order_list);
        order_t* matched_order = &matched_node->order;

        if (is_matched) {
            process_match(amended_order, matched_order, data);
                update_levels(*amended_order, -1, data);
                update_levels(*matched_order, -1, data);
        }
    }

    return TRUE;
}

// CANCEL <ID>;
// Handles the cancel message
// Returns false if the cancellation fails
bool cancel(char* command, int* tokens, int trader_id, exchange_data_t* data) {
    char id_string[BUFFER_SIZE];
    get_token(command, tokens, 1, id_string);
    order_t removed_order = {0};

    if (!is_number(id_string)) {
        send_invalid_signal(trader_id, data);
        return FALSE;
    }

    // Checks if the node exists and is valid
    bool is_valid = cancel_order(data->order_list, &removed_order, trader_id, atoi(id_string), data->product_count);
    if (!is_valid) {
        send_invalid_signal(trader_id, data);
        return FALSE;
    }

    // Updates levels and sends signals to each trader
    update_levels(removed_order, -1, data);
    send_market_signals(removed_order, trader_id, CANCELLED, data);

    return TRUE;
}

// Responds to a trader's message
void respond(char* command, int trader_id, exchange_data_t* data) {
    int tokens[MAX_TOKENS];
    char buffer[BUFFER_SIZE];

    char output_buffer[OUTPUT_BUFFER_SIZE];
    sprintf(output_buffer, " [T%d] Parsing command: <%s>\n", trader_id, command);
    pex_print(output_buffer);

    // Splits the message into tokens
    int token_count = split_instruction(command, tokens);
    bool is_valid_outcome = FALSE;
    get_token(command, tokens, 0, buffer);

    // Chooses the instruction based off the first token
    // Checks that there are the correct amount of tokens
    // Otherwise an invalid signal is sent to the sending trader
    if (strcmp(buffer, "BUY") == 0) {
        if (token_count == 5) {
            is_valid_outcome = buy_sell(command, tokens, TRUE, trader_id, data);
        }
        else {
            send_invalid_signal(trader_id, data);
        }
    }
    else if (strcmp(buffer, "SELL") == 0) {
        if (token_count == 5) {
            is_valid_outcome = buy_sell(command, tokens, FALSE, trader_id, data);
        }
        else {
            send_invalid_signal(trader_id, data);
        }
    }
    else if (strcmp(buffer, "AMEND") == 0) {
        if (token_count == 4) {
            is_valid_outcome = amend(command, tokens, trader_id, data);
        }
        else {
            send_invalid_signal(trader_id, data);
        }
    }
    else if (strcmp(buffer, "CANCEL") == 0) {
        if (token_count == 2) {
            is_valid_outcome = cancel(command, tokens, trader_id, data);
        }
        else {
            send_invalid_signal(trader_id, data);
        }
    }

    // If the message was valid, the orderbook and positions are printed
    if (is_valid_outcome) {
        print_orderbook(data); 
        print_positions(data); 
    }  
}

// Creates the product array
int initialise_products(FILE* product_file, product_t** products) {
    char file_buffer[BUFFER_SIZE];
    
    // Reads data from the product file
    fgets(file_buffer, BUFFER_SIZE, product_file);
    file_buffer[strlen(file_buffer) - 1] = '\0';
    int product_count = atoi(file_buffer);
    int product_id = 0;

    // Array size is dynamically allocated based on the number of products
    *products = calloc(product_count, sizeof(product_t));

    char output[OUTPUT_BUFFER_SIZE];
    sprintf(output, " Trading %d products:", product_count);
    pex_print(output);

    // Each product in the file is added to the products array
    while (fgets(file_buffer, BUFFER_SIZE, product_file)) {
        product_t* product = &(*products)[product_id];
        file_buffer[strlen(file_buffer) - 1] = '\0';
        sprintf(product->name, "%s", file_buffer);
        printf(" %s", product->name);
        product->id = product_id;
        product_id++;
    }
    printf("\n");

    return product_count;
}

int main(int argc, char **argv) {

    pex_print(" Starting\n");

	// Setting up traders
    trader_count = argc - 2;
    if (trader_count < 1) {
        return 1;
    }

    // Setting up signal handlers
    // SIGUSR1 - For communication between traders and exchange via FIFOs
    struct sigaction sigusr1_handler = {0};
    sigusr1_handler.sa_flags = SA_SIGINFO;
    sigusr1_handler.sa_sigaction = &handle_sigusr1;
    sigaction(SIGUSR1, &sigusr1_handler, NULL);

    // SIGCHLD - For communication when a trader terminates
    struct sigaction sigchld_handler = {0};
    sigchld_handler.sa_flags = SA_SIGINFO;
    sigchld_handler.sa_sigaction = &handle_sigchld;
    sigaction(SIGCHLD, &sigchld_handler, NULL);

    // Setting up the global signal queue
    signal_data.read_index = 0;
    signal_data.write_index = 0;
    signal_data.signal_count = 0;
    signal_data.signals = calloc(trader_count, sizeof(signal_t));

    // Reading product file
    FILE* product_file = fopen(argv[1], "r");
    product_t* products;
    int product_count = initialise_products(product_file, &products);

    char output[OUTPUT_BUFFER_SIZE];

    // Trader setup
    char id[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    int* trader_fds = calloc(trader_count, sizeof(int));
    bool* trader_status = calloc(trader_count, sizeof(bool));
    int active_traders = trader_count;
    trader_data_t* traders = calloc(trader_count, sizeof(trader_data_t));

    // Creating FIFO file descriptor storage
    char exchange_name[BUFFER_SIZE];
    char trader_name[BUFFER_SIZE];
    int* exchange_pipes = calloc(trader_count, sizeof(int));
    int* trader_pipes = calloc(trader_count, sizeof(int));


    for (int i = 0; i < trader_count; i++) {

        // Creating named pipes
        sprintf(exchange_name, FIFO_EXCHANGE, i);
        mkfifo(exchange_name, S_IRWXU);
        sprintf(output, " Created FIFO %s\n", exchange_name);
        pex_print(output);

        sprintf(trader_name, FIFO_TRADER, i);
        mkfifo(trader_name, S_IRWXU);
        sprintf(output, " Created FIFO %s\n", trader_name);
        pex_print(output);

        // Creating traders
        sprintf(id, "%d", i);
        string_slice(argv[i + 2], filename, 2, -1);
        trader_status[i] = TRUE;
        sprintf(output, " Starting trader %d (%s)\n", i, argv[i + 2]);
        pex_print(output);
        trader_fds[i] = fork();
        if (trader_fds[i] == 0) {
            execl(filename, filename, id, NULL);
        }

        // Connecting to named pipies
        exchange_pipes[i] = open(exchange_name, O_WRONLY);
        sprintf(output, " Connected to %s\n", exchange_name);
        pex_print(output);

        trader_pipes[i] = open(trader_name, O_RDONLY);
        sprintf(output, " Connected to %s\n", trader_name);
        pex_print(output);

        // Creating a trader structure
        trader_data_t trader = {
            .id = i,
            .fd = trader_fds[i],
            .product_counts = calloc(product_count, sizeof(int)),
            .product_costs = calloc(product_count, sizeof(long)),
            .next_order_id = 0
        };
        traders[i] = trader;
    }

    // Opening the market
    char* message = "MARKET OPEN;";
    int message_length = strlen(message);
    for (int i = 0; i < trader_count; i++) {
        write(exchange_pipes[i], message, message_length);
        kill(trader_fds[i], SIGUSR1);
    }

    // Creating the order linked list
    node_t order_array[MAX_ORDERS] = {0};
    node_linked_list_t order_list = {
        .head = NULL,
        .order_array = order_array
    };

    // Creating the exchange data structure
    exchange_data_t data = {
        .trader_pipes = trader_pipes,
        .exchange_pipes = exchange_pipes,
        .product_count = product_count,
        .products = products,
        .trader_count = trader_count,
        .trader_status = trader_status,
        .order_list = &order_list,
        .traders = traders
    };

    // Waiting for signals
    char buffer[BUFFER_SIZE];
    while (active_traders > 0) {

        // Passively polling until a signal is received
        pause();

        // Processing signals until there are none left
        while (signal_data.signal_count > 0) {

            // Finding the signal and its corresponding trader
            signal_t* signal = &signal_data.signals[signal_data.read_index];
            int trader_id = find_trader_id(trader_fds, trader_count, signal->trader_fd);

            // When SIGUSR1 is sent
            if (signal->signal_type == 1) {

                // Checking for messages and responding
                if (read(trader_pipes[trader_id], buffer, BUFFER_SIZE) > 0 && trader_status[trader_id]) {
                    if (clean_command(buffer)) {
                        respond(buffer, trader_id, &data);
                    }
                    else {
                        send_invalid_signal(trader_id, &data);
                    }
                }

                // Moving along the queue
                signal->signal_type = 0;
                signal->trader_fd = 0;
                signal_data.read_index = (signal_data.read_index + 1) % trader_count;
            }

            // WHEN SIGCHLD is sent
            else if (signal->signal_type == 2) {

                // Checking if the trader is currently active and disconnecting it
                if (trader_status[trader_id]) {
                    sprintf(output, " Trader %d disconnected\n", trader_id);
                    pex_print(output);
                    trader_status[trader_id] = FALSE;
                    active_traders--;
                }

                // Moving along the queue
                signal->signal_type = 0;
                signal->trader_fd = 0;
                signal_data.read_index = (signal_data.read_index + 1) % trader_count;
            }

            signal_data.signal_count--;
        }
    }

    // After all traders have disconnected, the trading is complete
    pex_print(" Trading completed\n");
    sprintf(output, " Exchange fees collected: $%ld\n", data.fees);
    pex_print(output);

    // Cleaning up all trader FIFOs and dynamically allocated memory
    for (int i = 0; i < trader_count; i++) {
        close(exchange_pipes[i]);
        close(trader_pipes[i]);
        free(traders[i].product_counts);
        free(traders[i].product_costs);

        sprintf(exchange_name, FIFO_EXCHANGE, i);
        unlink(exchange_name);
        sprintf(trader_name, FIFO_TRADER, i);
        unlink(trader_name);
    }

    // Cleaning up other dynamically allocated memory
    free(trader_fds);
    free(trader_status);
    free(trader_pipes);
    free(exchange_pipes);
    free(products);
    free(traders);
    free(signal_data.signals);

	return 0;
}
