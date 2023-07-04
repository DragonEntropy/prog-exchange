#include "pe_common.h"

// Takes a slice of a string between two indicies
void string_slice(char* input, char* output, int start_index, int end_index) {
    int i;
    for (i = 0; i < BUFFER_SIZE - 1; i++) {
        output[i] = input[start_index + i];
        if (output[i] == '\0' || start_index + i - 1 == end_index) {
            break;
        }
    }
    output[i + 1] = '\0';
}

// Tokenises a string by splitting spaces
int split_instruction(char* instruction, int tokens[MAX_TOKENS]) {
    tokens[0] = 0;
    int i = 0;
    int t = 1;
    while (instruction[i] != '\0') {

        if (instruction[i] == ' ') {
            if (t == MAX_TOKENS) {
                return -1;
            }

            tokens[t] = i + 1;
            t++;
        }
        i++;  
    }

    return t;
}

// Gets a token from a tokenised string
int get_token(char* command, int tokens[MAX_TOKENS], int index, char* buffer) {
    int i = 0;
    int offset = tokens[index];
    while (command[i + offset] != ' ' && command[i + offset] != '\0') {
        buffer[i] = command[i + offset];
        i++;
    }
    buffer[i] = '\0';

    return i;
}

// Checks if a string is a number
bool is_number(char* input) {
    while (*input != '\0') {
        if (!isdigit(*input)) {
            return FALSE;
        }
        input++;
    }

    return TRUE;
}