CC=gcc
CFLAGS=-c -std=c99 -Wall -Werror -fpic -g
LDFLAGS=-g
LDLIBS=-ledit -lm
OUTPUT_DIR=bin

APP=$(OUTPUT_DIR)/clisp
LIB=$(APP).so
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:%.c=$(OUTPUT_DIR)/%.o)
DRIVER_SOURCE=main.c
DRIVER_OBJECT=$(OUTPUT_DIR)/main.o
LIB_SOURCES=$(filter-out $(DRIVER_SOURCE), $(SOURCES))
LIB_OBJECTS=$(filter-out $(DRIVER_OBJECT), $(OBJECTS))

$(APP): $(DRIVER_OBJECT) $(LIB)
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

$(DRIVER_OBJECT): $(LIB_OBJECTS)
	$(CC) $(CFLAGS) $(DRIVER_SOURCE) -o $@

$(LIB): $(LIB_OBJECTS)
	$(CC) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@

$(LIB_OBJECTS): $(OUTPUT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OUTPUT_DIR)

.PHONY: clean

$(shell mkdir -p $(OUTPUT_DIR))
