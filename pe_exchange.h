#ifndef PE_EXCHANGE_H
#define PE_EXCHANGE_H

#include "orders.h"

#define LOG_PREFIX "[PEX]"

// Represents the different trader messages
typedef enum message_type_t {
	ACCEPTED = 1,
	AMENDED = 2,
	CANCELLED = 3,
	INVALID = 0
} message_type_t;

// Stores the data for a single trader
typedef struct trader_data_t {
	int id;
	int fd;
	int* product_counts;
	long* product_costs;
	int next_order_id;
} trader_data_t;

// Stores the state of the exchange that is passed between functions
typedef struct exchange_data_t {
	int* trader_pipes;
	int* exchange_pipes;
	int product_count;
	product_t* products;
	int trader_count;
	bool* trader_status;
	node_linked_list_t* order_list;
	trader_data_t* traders;
	long fees;
} exchange_data_t;

// Stores an instance of a signal
typedef struct signal_t {
	int trader_fd;
	int signal_type;
} signal_t;

// A queue for the signals
typedef struct signal_data_t {
	int read_index;
	int write_index;
	int signal_count;
	signal_t* signals;
} signal_data_t;

void pex_print(char* output);
void format_order(char* output, order_t* order, int quantity, int order_count);
void format_position(char* output, trader_data_t trader, exchange_data_t* data, int product_id, bool is_last);
void print_orderbook(exchange_data_t* data);
void print_positions(exchange_data_t* data);
void handle_sigusr1(int sig, siginfo_t* info, void* context);
void handle_sigchld(int sig, siginfo_t* info, void* context);
int find_product_id(product_t* products, int product_count, char* name);
int find_trader_id(int* trader_fds, int trader_count, int trader_fd);
bool clean_command(char* command);
void send_invalid_signal(int sender_id, exchange_data_t* data);
void send_market_signals(order_t order, int sender_id, message_type_t message, exchange_data_t* data);
void send_fill_signals(order_t buyer_order, order_t seller_order, int buyer_id, int seller_id, int quantity, exchange_data_t* data);
void update_levels(order_t order, int sign, exchange_data_t* data);
void process_match(order_t* added_order, order_t* matched_order, exchange_data_t* data);
bool buy_sell(char* command, int* tokens, bool is_buy, int trader_id, exchange_data_t* data);
bool amend(char* command, int* tokens, int trader_id, exchange_data_t* data);
bool cancel(char* command, int* tokens, int trader_id, exchange_data_t* data);
void respond(char* command, int trader_id, exchange_data_t* data);
int initialise_products(FILE* product_file, product_t** products);
int main(int argc, char **argv);

#endif
