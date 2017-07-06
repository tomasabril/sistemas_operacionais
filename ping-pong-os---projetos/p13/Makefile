# Tomas Abril 2017

CC = gcc
CFLAGS = -g -I. -lm -lrt

mqueue: pingpong.o queue.o pingpong-disco.o
	$(CC) $(CFLAGS) -o execut pingpong.c queue.c pingpong-disco.c

clean:
	rm *.o execut*

#-Wall -Wextra
