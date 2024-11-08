CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -pthread -O3

TARGETS = selection numbers glazer web arrays

all: $(TARGETS)

selection: selection.c
	$(CC) $(CFLAGS) -o $@ $<

numbers: numbers.c
	$(CC) $(CFLAGS) -o $@ $<

glazer: glazer.c
	$(CC) $(CFLAGS) -o $@ $<

web: web.c
	$(CC) $(CFLAGS) -o $@ $<

arrays: arrays.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o *.log

.PHONY: all clean