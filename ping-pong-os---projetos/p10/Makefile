# Tomas Abril 2017

CC = gcc
CFLAGS = -g -I.

sema: pingpong.o queue.o pingpong-semaphore.o
	$(CC) -o execut-semaphore pingpong.c queue.c pingpong-semaphore.c

race: pingpong.o queue.o pingpong-racecond.o
		$(CC) -o execut-racecond pingpong.c queue.c pingpong-racecond.c

prodcons: pingpong.o queue.o pingpong-prodcons.o
		$(CC) -o execut-prodcons pingpong.c queue.c pingpong-prodcons.c

clean:
	rm *.o execut-semaphore execut-racecond execut-prodcons

#-Wall -Wextra
