CC=gcc
CFLAGS=-I ../headers -I ../phase1/headers -I include
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o) $(wildcard phase1/*.o)
DEP = $(OBJ:.o=.d)

-include $(DEP)

phase2: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ) phase2 $(DEP)
