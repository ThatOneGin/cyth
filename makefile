export CC = cc
export COMPFLAGS = -ggdb -Wall -Wextra -pedantic -std=c99
CFLAGS = $(COMPFLAGS) -I include/
BIN = cyth
export CORESRC = src/caux.c src/cbuiltin.c src/cchunk.c \
				src/cfunc.c src/cgc.c src/cio.c \
				src/clex.c src/clex_cx.c src/clex_cyth.c \
				src/cmem.c src/cobject.c src/copcode.c \
				src/cparser_cx.c src/cparser_cyth.c src/cstate.c \
				src/cstring.c src/cvm.c
export COREOBJECT = build/caux.o build/cbuiltin.o build/cchunk.o \
						 build/cfunc.o build/cgc.o build/cio.o \
						 build/clex.o build/clex_cx.o build/clex_cyth.o \
						 build/cmem.o build/cobject.o build/copcode.o \
						 build/cparser_cx.o build/cparser_cyth.o build/cstate.o \
						 build/cstring.o build/cvm.o

SOURCE = $(CORESRC) src/main.c
OBJECT = $(COREOBJECT) build/main.o

all: $(BIN)

$(BIN): $(OBJECT)
	$(CC) -o $(BIN) $(CFLAGS) $(OBJECT)

build/%.o: src/%.c
	$(CC) -c $(CFLAGS) $(MYCFLAGS) -o $(patsubst src/%.c, build/%.o, $<) $<

lib:
	$(MAKE) -C lib/

lib-clean:
	$(MAKE) -C lib/ clean

clean:
	rm $(OBJECT)
	rm $(BIN)

.PHONY: all clean lib lib-clean