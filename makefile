CC = gcc
CFLAGS = -ggdb -Wall -Wextra -pedantic -std=c99
BUILD = build
SRC_FILES = $(wildcard src/*.c) $(wildcard src/**/*.c)
OBJ_FILES = $(patsubst src/%.c, %.o, $(SRC_FILES))
BIN = cyth

all: $(BIN)
	mv *.o build/

$(BIN): $(OBJ_FILES)
	$(CC) -o $(BIN) $(OBJ_FILES)

%.o: src/%.c
	$(CC) -c $(CFLAGS) $<

clean:
	rm *.o
	rm $(BIN)
