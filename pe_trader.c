#include "pe_trader.h"

int signal_received = 0;

void handle_signal(int sig) {
    signal_received = 1;
}

int split_instruction(char* instruction, int tokens[MAX_TOKENS]) {
    tokens[0] = 0;
    int i = 0;
    int t = 1;
    while (instruction[i] != ';') {
        if (instruction[i] == ' ') {
            tokens[t] = i + 1;
            t++;
        }
        i++;
    }

    return t;
}

int get_token(char* command, int tokens[MAX_TOKENS], int index, char* buffer) {
    int i = 0;
    int offset = tokens[index];
    while (command[i + offset] != ' ' && command[i + offset] != ';') {
        buffer[i] = command[i + offset];
        i++;
    }
    buffer[i] = '\0';

    return i;
}

void buy(char* product, char* quantity, char* price, Trader* trader) {
    int int_quantity = atoi(quantity);
    if (int_quantity <= 0 || int_quantity >= 1000) {
        trader->market_open = 0;
        return;
    }

    char message[BUFFER_SIZE];
    sprintf(message, "BUY %d %s %s %s;", trader->order_id, product, quantity, price);
    write(trader->trader_fd, message, strlen(message));
    kill(trader->ppid, SIGUSR1);
    trader->order_id++;
}

void respond(char* command, Trader* trader) {
    int tokens[MAX_TOKENS] = {0};
    int token_count = split_instruction(command, tokens);

    char buffer[BUFFER_SIZE];

    get_token(command, tokens, 0, buffer);
    if (strcmp(buffer, "MARKET") == 0) {
        get_token(command, tokens, 1, buffer);
        if (strcmp(buffer, "SELL") == 0) {
            if (token_count == 5) {
                char product[BUFFER_SIZE];
                char quantity[BUFFER_SIZE];
                char price[BUFFER_SIZE];
                get_token(command, tokens, 2, product);
                get_token(command, tokens, 3, quantity);
                get_token(command, tokens, 4, price);
                buy(product, quantity, price, trader);
            }
        }
    }
}

int check_command(char* command, char* input) {
    int i = 0;
    while (command[i] != ';') {
        if (command[i] != input[i]) {
            return 0;
        }
        i++;
    }
    if (input[i] != ';') {
        return 0;
    }
    return 1;
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

    // Setting up the trader
    Trader trader = {0};
    trader.order_id = 0;
    trader.ppid = getppid();
    trader.market_open = 0;

    // Creating signal handler
    struct sigaction sa;
    sa.sa_handler = &handle_signal;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
    
    // Connecting to FIFOs
    char trader_name[BUFFER_SIZE];
    char exchange_name[BUFFER_SIZE];
    sprintf(exchange_name, FIFO_EXCHANGE, atoi(argv[1]));
    trader.exchange_fd = open(exchange_name, O_RDONLY);
    sprintf(trader_name, FIFO_TRADER, atoi(argv[1]));
    trader.trader_fd = open(trader_name, O_WRONLY);
    // printf("Exchange: %d, Trader: %d\n", trader.exchange_fd, trader.trader_fd);
    
    // Waiting for market open:
    char command_buffer[BUFFER_SIZE];
    while (!trader.market_open) {
        pause();
        if (signal_received) {
            signal_received = 0;
            read(trader.exchange_fd, command_buffer, BUFFER_SIZE);
            if (check_command(command_buffer, "MARKET OPEN;")) {
                trader.market_open = 1;
            }
        }
    }

    while (trader.market_open) {
        pause();
        if (signal_received) {
            signal_received = 0;
            read(trader.exchange_fd, command_buffer, BUFFER_SIZE);
            respond(command_buffer, &trader);
        }
    }
    
    close(trader.trader_fd);
    close(trader.exchange_fd);
}