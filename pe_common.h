#ifndef PE_COMMON_H
#define PE_COMMON_H

#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <math.h>
#include <ctype.h>

// File name format strings
#define FIFO_EXCHANGE "/tmp/pe_exchange_%d"
#define FIFO_TRADER "/tmp/pe_trader_%d"

// Buffer sizes
#define BUFFER_SIZE (64)
#define OUTPUT_BUFFER_SIZE (256)
#define PRODUCT_BUFFER_SIZE (32)

// Other key constants
#define FEE_PERCENTAGE 1
#define MAX_TOKENS (5)
#define MAX_PRICE (1000000)
#define MAX_QUANTITY (1000000)

// Standard boolean enum
typedef enum {
	TRUE = 1,
	FALSE = 0
} bool;

void string_slice(char* input, char* output, int start_index, int end_index);
int split_instruction(char* instruction, int tokens[MAX_TOKENS]);
int get_token(char* command, int tokens[MAX_TOKENS], int index, char* buffer);
bool is_number(char* input);

#endif
