CC=gcc
CFLAGS  += -O0 -std=gnu17 -Wall -pedantic -g

CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
 -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
 -Wconversion -Wunreachable-code -Wformat=2

COMMON += 
HEADERS +=
APP = main

## ---------------------------------------------------
## --------- Template stuff : Do not touch -----------

all: $(APP)

%.o: %.c $(HEADERS)

main:
	clang -ggdb3 *.cc -o main

clean:
	@rm -f $(APP) *.o
