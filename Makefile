CC=gcc
CFLAGS=-Os
DEPS=hexdump.h
OBJS=gopher.o hexdump.o

all: gopher hexdump-echo

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

gopher: gopher.o hexdump.o
	$(CC) -o gopher hexdump.o gopher.o $(CFLAGS) -levent_core

hexdump-echo: hexdump-echo.o
	$(CC) -o hexdump-echo hexdump-echo.o $(CFLAGS) -levent_core

clean:
	rm gopher hexdump-echo *.o
