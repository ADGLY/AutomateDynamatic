CC=gcc
CFLAGS  += -O0 -std=gnu17 -Wall -pedantic

CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
 -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
 -Wconversion -Wunreachable-code -Wformat=2 -Wformat-truncation \
 -Wstringop-truncation -Wformat-overflow -Wstringop-overflow \
 -Wunused-result

APP = main

all: $(APP)

main: *.c
	$(CC) -ggdb3 $(CFLAGS) *.c -o main -lpcreposix -lpcre

clean:
	@rm -f $(APP) *.o
