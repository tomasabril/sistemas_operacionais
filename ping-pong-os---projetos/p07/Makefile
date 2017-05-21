CC = gcc
CFLAGS = -Wall -Wextra -g -I.

Debug: pingpong.o queue.o pingpong-maintask.o
	$(CC) -o execut-maintask pingpong.c queue.c pingpong-maintask.c

clean:
	rm *.o execut execut-maintask
