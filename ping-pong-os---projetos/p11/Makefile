# Tomas Abril 2017

CC = gcc
CFLAGS = -g -I.

barr: pingpong.o queue.o pingpong-barrier.o
	$(CC) -o execut pingpong.c queue.c pingpong-barrier.c

clean:
	rm *.o execut

#-Wall -Wextra
