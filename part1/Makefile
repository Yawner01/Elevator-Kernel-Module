BIN := bin
EXECUTABLE := part1
EMPTY_EXEC := empty

INCS := -Iinclude/
DIRS := $(BIN)/
EXEC := $(BIN)/$(EXECUTABLE)
EMPTY := $(BIN)/$(EMPTY_EXEC)

CC := gcc
CFLAGS := -g -Wall -std=gnu99 -D_GNU_SOURCE $(INCS)
LDFLAGS :=

all: $(EXEC) $(EMPTY)

$(EXEC): part1.c
	$(CC) $(CFLAGS) $< -o $(EXEC)

$(EMPTY): empty.c
	$(CC) $(CFLAGS) $< -o $(EMPTY)

part1: $(EXEC)
	$(EXEC)

empty: $(EMPTY)
	$(EMPTY)

clean:
	rm -f $(EXEC) $(EMPTY)

$(shell mkdir -p $(DIRS))

.PHONY: part1 empty clean all