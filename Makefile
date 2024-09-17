CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -pthread -O3

TARGETS = selection numbers glazer web

all: $(TARGETS)

selection: selection.c
	$(CC) $(CFLAGS) -o $@ $<

numbers: numbers.c
	$(CC) $(CFLAGS) -o $@ $<

glazer: glazer.c
	$(CC) $(CFLAGS) -o $@ $<

web: web.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean