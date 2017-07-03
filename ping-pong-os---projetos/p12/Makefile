# Tomas Abril 2017

CC = gcc
CFLAGS = -g -I. -lm

mqueue: pingpong.o queue.o pingpong-mqueue.o
	$(CC) $(CFLAGS) -o execut pingpong.c queue.c pingpong-mqueue.c

clean:
	rm *.o execut*

#-Wall -Wextra
