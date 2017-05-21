CC = gcc
CFLAGS = -Wall -Wextra -g -I.

contab: pingpong.o queue.o pingpong-contab.o
	$(CC) -o execut-contab pingpong.c queue.c pingpong-contab.c

contab-prio: pingpong.o queue.o pingpong-contab-prio.o
	$(CC) -o execut-contab-prio pingpong.c queue.c pingpong-contab-prio.c

clean:
	rm *.o execut execut-contab execut-contab-prio
