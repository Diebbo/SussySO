CC=gcc
CFLAGS=-I headers -I phase1/headers
DEPS = headers/*.h phase1/headers/*.h
OBJ = $(patsubst %.c, %.o, $(wildcard src/*.c))

%.o: %.c $(DEPS)
    $(CC) -c -o $@ $< $(CFLAGS)

myprogram: $(OBJ)
    $(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
    rm -f $(OBJ) myprogram