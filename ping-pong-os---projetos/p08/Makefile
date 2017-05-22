CC = gcc
CFLAGS = -Wall -Wextra -g -I.

all: pingpong.o queue.o pingpong-join.o
	$(CC) -o execut-join pingpong.c queue.c pingpong-join.c

clean:
	rm *.o execut-join
