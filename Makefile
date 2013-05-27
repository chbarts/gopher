CC=gcc
CFLAGS=-Os
DEPS=hexdump.h
OBJS=gopher.o hexdump.o

all: gopher

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

gopher: gopher.o hexdump.o
	$(CC) -o gopher hexdump.o gopher.o $(CFLAGS) -levent

clean:
	rm gopher *.o
