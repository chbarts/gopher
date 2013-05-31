CC=gcc
CFLAGS=-Os
DEPS=hexdump.h
OBJS=gopher.o hexdump.o

all: gopher hexdump-echo

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

gopher: gopher.o
	$(CC) -o gopher gopher.o -levent_core

hexdump-echo: hexdump-echo.o
	$(CC) -o hexdump-echo hexdump-echo.o -levent_core

clean:
	rm gopher hexdump-echo *.o
