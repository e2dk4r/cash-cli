BINARY = cash

SOURCES = $(sort $(wildcard src/*.c))
OBJECTS = $(SOURCES:.c=.o)
	
CC ?= gcc
CFLAGS = -std=c99 -Wall -O2 -I include
LDFLAGS = -lcurl -ljansson

all: $(SOURCES) $(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BINARY) $(OBJECTS)

.PHONY: all clean
