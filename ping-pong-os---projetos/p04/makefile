CC = gcc
CFLAGS = -Wall -Wextra -g -I.

tasks1: pingpong.o queue.o pingpong-scheduler.o
	$(CC) -o execut pingpong.c queue.c pingpong-scheduler.c

clean:
	rm *.o execut
