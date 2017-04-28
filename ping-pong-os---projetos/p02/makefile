CC = gcc
CFLAGS = -Wall -Wextra -g -I.

tasks1: pingpong.o pingpong-tasks1.o
	$(CC) -o tasks1 pingpong.c pingpong-tasks1.c

tasks2: pingpong.o pingpong-tasks2.o
	$(CC) -o tasks2 pingpong.c pingpong-tasks2.c

tasks3: pingpong.o pingpong-tasks3.o
	$(CC) -o tasks3 pingpong.c pingpong-tasks3.c

clean:
	rm *.o contexts tasks1 tasks2 tasks3
