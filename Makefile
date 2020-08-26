CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 $(shell pkg-config --cflags toxcore) #TODO: switch to c99
LDFLAGS = $(shell pkg-config --libs toxcore)

SRC = $(wildcard src/*.c ) $(wildcard src/*/*.c) third-party/minini/dev/minIni.c
OBJ = $(SRC:.c=.o)
HEADERS = $(wildcard src/*.h) $(wildcard src/*/*.h)

DEBUG = 1
PREFIX = /usr/local
EXECUTABLE = toxirc

ifeq ($(DEBUG), 1)
	CFLAGS += -g
endif

all: $(SRC) $(EXECUTABLE)

$(EXECUTABLE): $(OBJ) $(HEADERS)
	$(CC) $(OBJ) $(LDFLAGS) -o $(EXECUTABLE)

.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm src/*.o src/*/*.o $(EXECUTABLE)

rebuild: clean all

install: all
	@echo "Installing binary..."
	install -m 0755 $(EXECUTABLE) $(PREFIX)/bin/$(EXECUTABLE)

deps:
	@git submodule init
	@git submodule update

.PHONY: all clean
