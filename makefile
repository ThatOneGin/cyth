CC = cc
CFLAGS = -ggdb -Wall -Wextra -pedantic -std=c99 -I src/
BIN = cyth
SOURCE = $(wildcard src/*.c)
OBJECT = $(patsubst %.c, build/%.o, $(notdir $(SOURCE)))

all: $(BIN)

$(BIN): $(OBJECT)
	$(CC) -o $(BIN) $(CFLAGS) $(OBJECT)

build/%.o: src/%.c
	$(CC) -c $(CFLAGS) $(MYCFLAGS) -o $(patsubst src/%.c, build/%.o, $<) $<

clean:
	rm $(OBJECT)
	rm $(BIN)
