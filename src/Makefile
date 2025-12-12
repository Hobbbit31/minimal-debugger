CC = gcc

mydbg: main.c child.c parent.c include/core.h include/launch.h include/wait.h
	$(CC) -o mydbg main.c child.c parent.c include/launch.c include/wait.c

all: mydbg

clean:
	rm -f mydbg
