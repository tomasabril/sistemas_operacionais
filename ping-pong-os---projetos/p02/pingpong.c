#include "pingpong.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

#define STACKSIZE 32768		/* tamanho de pilha das threads */
//#define DEBUG true

int id = 0;	//vai somando pra cada tarefa ter id diferente
task_t main_tsk;	// main task
task_t *current_tsk; // task sendo executada no momento

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init ()
{
    main_tsk.tid = id++;	//comeca com id 0
    current_tsk = &main_tsk;

    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0) ;

#ifdef DEBUG
    printf("inicializando main com id %d \n", (&main_tsk)->tid);
#endif
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg)			// argumentos para a tarefa
{

    if(task != NULL) {
        getcontext (&(task->context));	//pega contexto atual

        char *stack = malloc (STACKSIZE);
        //fazendo o contexto
        if (stack) {
            task->context.uc_stack.ss_sp = stack;
            task->context.uc_stack.ss_size = STACKSIZE;
            task->context.uc_stack.ss_flags = 0;
            task->context.uc_link = 0;
            task->tid = id++;
        } else {
            perror ("stack nao pode ser criado ");
            return -1;
        }
    } else {
        perror ("tarefa veio com erro ");
        return -1;
    }

    //liga a funcao a este contexto
    makecontext (&task->context, (void*)(*start_func), 1, arg);

#ifdef DEBUG
    printf("task_create: criou a tarefa %d\n", task->tid);
#endif

    return task->tid;
    //return 0;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{
    task_t *atual_tsk;
    if (task != NULL) {
        atual_tsk = current_tsk;
        current_tsk = task;
#ifdef DEBUG
        printf("task_switch: trocando contexto %d -> %d\n", atual_tsk->tid, current_tsk->tid);
#endif

        swapcontext(&atual_tsk->context, &current_tsk->context);
        return 0;
    } else {

        return -1;
    }
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
#ifdef DEBUG
    printf("task_exit: tarefa %d sendo encerrado com codigo %d\n", current_tsk->tid, exitCode);
#endif
    //current_tsk = &main_tsk;
    task_switch(&main_tsk);
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id ()
{
    int task_id_at = current_tsk->tid;
    return task_id_at;
}
























