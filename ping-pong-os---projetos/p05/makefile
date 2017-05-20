CC = gcc
CFLAGS = -Wall -Wextra -g -I.

preemp: pingpong.o queue.o pingpong-preempcao.o
	$(CC) -o execut pingpong.c queue.c pingpong-preempcao.c

signal: signal.o
	$(CC) -o signal signal.c

timer: timer.o
	$(CC) -o timer timer.c

clean:
	rm *.o execut signal timer
