CC = gcc
CFLAGS = -Wall -std=c99 -g -c

simcpu: simcpu.o
	$(CC) simcpu.o -o simcpu

caltool.o: simcpu.c
	$(CC) $(CFLAGS) simcpu.c

clean:
	rm *o simcpu 