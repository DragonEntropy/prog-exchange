TARGET = pe_exchange
TRADER = pe_trader

CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak -D_POSIX_C_SOURCE=199309L
LDFLAGS=-lm
SRC=pe_exchange.c orders.c pe_common.c
OBJ=$(SRC:.c=.o)

all: $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(CFLAGS) $(LDFLAGS)
	$(CC) -o $(TRADER) $(TRADER).c $(CFLAGS) $(LDFLAGS)


$(OBJ) : $(SRC)
	$(CC) -c $(SRC) $(CFLAGS) $(LDFLAGS)


.PHONY: clean
clean:
	rm -f *.o $(TARGET) $(TRADER)