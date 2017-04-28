CC = gcc
CFLAGS = -Wall -Wextra -g -I.

tasks1: pingpong.o queue.o pingpong-dispatcher.o
	$(CC) -o execut pingpong.c queue.c pingpong-dispatcher.c

clean:
	rm *.o execut
