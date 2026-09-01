BIN = libcyth.so
CFLAGS = $(COMPFLAGS) -I ../include/
SOURCE = cyth.c $(patsubst src/%, ../src/%, $(CORESRC))
OBJECT = cyth.o $(notdir $(COREOBJECT))

all: $(BIN)
	mv $(BIN) ..

$(BIN): $(OBJECT)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^

%.o: ../src/%.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	rm $(OBJECT)
	rm ../$(BIN)

.PHONY: all clean