// PingPongOS - PingPong Operating System
// Tomas Abril
// 2017_1

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pingpong.h"

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif


// variaveis
int buffer[5] = {};
int p_index = 0;
int c_index = 0;
task_t p1, p2, p3, c1, c2;
semaphore_t s_buffer, s_item, s_vaga;

void produtor(char *arg)
{
    int item;
    while (1)
    {
        task_sleep (1);
        item = rand()%100;

        sem_down(&s_vaga);
        sem_down(&s_buffer);

        // insere item no buffer
        buffer[p_index] = item;
        p_index = (++p_index) % 5;

        sem_up(&s_buffer);
        sem_up(&s_item);

        printf("%s produziu %d \n", arg, item);
    }
    task_exit(0);
}

void consumidor(char *arg)
{
    int item;
    while (1)
    {
        sem_down(&s_item);
        sem_down(&s_buffer);

        // retira item do buffer
        item = buffer[c_index];
        c_index = (++c_index) % 5;

        sem_up(&s_buffer);
        sem_up(&s_vaga);

        //print item
        printf("%s consumiu %d \n", arg, item);

        task_sleep(1);
    }
    task_exit(0);
}

int main(int argc, char const *argv[]) {
    printf ("Main INICIO\n") ;
    srand(time(NULL));

    pingpong_init () ;

    sem_create(&s_buffer, 1);
    sem_create(&s_item, 0);
    sem_create(&s_vaga, 5);

    task_create(&c1, consumidor, "\t\t c1");
    task_create(&c2, consumidor, "\t\t c2");

    task_create(&p1, produtor, "p1");
    task_create(&p2, produtor, "p2");
    task_create(&p3, produtor, "p3");

    task_join(&c1);
    task_join(&c2);
    task_join(&p1);
    task_join(&p2);
    task_join(&p3);

    // sem_destroy(&s_buffer);
    // sem_destroy(&s_item);
    // sem_destroy(&s_vaga);

    printf ("Main FIM\n") ;
    task_exit(0);
    exit(0);
}
