#ifndef PE_TRADER_H
#define PE_TRADER_H

#include "pe_common.h"

typedef struct trader_struct {
    int exchange_fd;
    int trader_fd;
    pid_t ppid;
    int order_id;
    int market_open;
} Trader;

void handle_signal(int sig);
int split_instruction(char* instruction, int tokens[MAX_TOKENS]);
int get_token(char* command, int tokens[MAX_TOKENS], int index, char* buffer);
void buy(char* product, char* quantity, char* price, Trader* trader);
void respond(char* command, Trader* trader);
int check_command(char* command, char* input);
int main(int argc, char ** argv);

#endif
