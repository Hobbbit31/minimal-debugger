CC = gcc

mydbg: main.c include/debug.h include/breakpoint.h
	$(CC) -o mydbg main.c include/debugger.c include/breakpoint.c

all: mydbg

clean:
	rm -f mydbg
