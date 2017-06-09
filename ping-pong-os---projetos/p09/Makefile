CC = gcc
CFLAGS = -g -I.

all: pingpong.o queue.o pingpong-sleep.o
	$(CC) -o execut pingpong.c queue.c pingpong-sleep.c

clean:
	rm *.o execut

#-Wall -Wextra 
