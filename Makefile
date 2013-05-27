CC=gcc
CFLAGS=-Os
OBJS=gopher.o

all: gopher

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

gopher: gopher.o
	$(CC) -o gopher gopher.o $(CFLAGS) -levent

clean:
	rm gopher *.o
