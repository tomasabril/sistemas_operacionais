#include "queue.h"
#include "pingpong.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

#include <signal.h>
#include <sys/time.h>


#define STACKSIZE 32768		/* tamanho de pilha das threads */ //copiei do p01, nao sei o porque desse tamanho em especifico
#define TRUE 1
#define FALSE 0

//#define DEBUG true
//#define DEBUG_JOIN true
//#define DEBUG_QUANTUM


int id = 0;	//vai somando pra cada tarefa ter id diferente
task_t main_tsk;	// main task
task_t *current_tsk; // task sendo executada no momento

task_t dispatcher;  //task que controla os despachamentos
task_t *ready_tasks = NULL; //lista de tarefas prontas
int userTasks = 0;  //contador de tarefas na fila

int a = -1;   //aging coeficient
int prio_min = -20; //essa é a task que executara antes
int prio_max = 20;  //ultima coisa a ser executada
int tick_u = 1000;
int quantum_max = 20;
int quantum = 0;    //essa variavel vai decrescendo para cada tarefa
unsigned int sysclock_ms = 0;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;
// estrutura de inicialização to timer
struct itimerval timer;

// minhas funções
task_t *scheduler_fcfs();
task_t *scheduler();
void dispatcher_body(void *arg);
void timer_tratador(int signum);


// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init ()
{
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);


    //------------ criando main ---------------------------------------------
    main_tsk.tid = id++;	//comeca com id 0
    main_tsk.init_time = 0;
    main_tsk.proc_time = 0;
    main_tsk.exec_time = 0;
    main_tsk.activations = 1;
    main_tsk.status = EXEC;
    main_tsk.wait_me_q = NULL;
    main_tsk.lock = FALSE;
    userTasks++;
    task_setprio(&main_tsk, 0);
    current_tsk = &main_tsk;

#ifdef DEBUG
    printf("inicializando main com id %d \n", (&main_tsk)->tid);
#endif

    task_create(&dispatcher, dispatcher_body, "dispatcher ");

    //-------- iniciando o temporizador --------------------------------------
    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = timer_tratador;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGALRM, &action, 0) < 0) {
        perror ("Erro em sigaction: ");
        exit (1) ;
    }
    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1 ;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = tick_u;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0) {
        perror ("Erro em setitimer: ");
        exit (1);
    }
#ifdef DEBUG
    printf("timer interval %d microsecs %d secs \n", tick_u, 0);
#endif
    //-------- ------------------------ --------------------------------------

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
            task->init_time = systime();
            task->proc_time = 0;
            task->exec_time = 0;
            task->activations = 0;
            task->status = READY;
            task->wait_me_q = NULL;
            task->lock = FALSE;

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

    task_setprio(task, 0);

    // colocando na fila as tarefas de usuario
    if(task->tid != 1) {
        userTasks++;
        queue_append((queue_t **) &ready_tasks, (queue_t *) task);
        task->my_queue = (queue_t **) &ready_tasks;
    }

#ifdef DEBUG
    printf("task_create: criou a tarefa %d\n", task->tid);
#endif

    return task->tid;

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

        // resetando quantum
        quantum = quantum_max;
        current_tsk->activations++;

        atual_tsk->status = READY;
        current_tsk->status = EXEC;

        swapcontext(&atual_tsk->context, &current_tsk->context);
        return 0;
    } else {

        return -1;
    }
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
    current_tsk->lock = TRUE;
#ifdef DEBUG
    printf("task_exit: tarefa %d sendo encerrado com codigo %d\n", current_tsk->tid, exitCode);
#endif

    unsigned int now = systime();
    unsigned int exec_time = now - current_tsk->init_time;
    current_tsk->exec_time = exec_time;

    printf("Task %d exit: exec_time %d ms, proc_time %d ms, %d activations\n", current_tsk->tid, exec_time, current_tsk->proc_time, current_tsk->activations);

    current_tsk->exit_code = exitCode;
    current_tsk->status = ENDED;

#if defined(DEBUG) || defined(DEBUG_JOIN)
    printf("status da tarefa encerrada = ");
    if(current_tsk->status == READY) {
        printf("READY");
    } else if(current_tsk->status == EXEC) {
        printf("EXEC");
    } else if(current_tsk->status == SUSP) {
        printf("SUSP");
    } else if(current_tsk->status == ENDED) {
        printf("ENDED");
    }
    printf("\n");
#endif

    // acordando tasks suspensas
#if defined(DEBUG_JOIN)
    int i=0;
#endif
    while (current_tsk->wait_me_q) {
#if defined(DEBUG_JOIN)
        printf("i=%d\n", i++);
#endif
        current_tsk->wait_me_q->status = READY;
        current_tsk->wait_me_q->my_queue = NULL;
        task_t * esperando = queue_remove((queue_t **)&(current_tsk->wait_me_q), (queue_t *)current_tsk->wait_me_q);

        queue_append ((queue_t **) &ready_tasks, (queue_t *) esperando);
    }

    current_tsk->lock = FALSE;

    if(current_tsk->tid != 1) {
        userTasks--;
        task_switch(&dispatcher);
    }

}

// retorna o identificador da tarefa corrente (main é 0)
int task_id ()
{
    int task_id_at = current_tsk->tid;
    return task_id_at;
}

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield ()
{
#ifdef DEBUG
    printf("task_yield: id da tarefa: %d\n", current_tsk->tid);
#endif

    // colocando na fila as tarefas de usuario
    if(current_tsk->tid != 1) {
        userTasks++;
        queue_append((queue_t **) &ready_tasks, (queue_t *) current_tsk);  //colocando na fila para esperar
        current_tsk->my_queue = (queue_t **) &ready_tasks; // atualiza sua variavel de fila
        current_tsk->status = READY;
    }

    task_switch(&dispatcher);   //volta pro dispatcher
}

void dispatcher_body(void *arg)
{
    userTasks = queue_size((queue_t *)ready_tasks);
    while ( userTasks > 0 ) {
        task_t *next = scheduler() ; // scheduler é uma função
        if (next) {
#ifdef DEBUG
            printf("dispatcher_body: proxima tarefa id: %d\n", next->tid);
            //queue_print("lista: ",(queue_t **)ready_tasks, task_print);
#endif
            // ações antes de lançar a tarefa "next", se houverem
            task_switch (next) ; // transfere controle para a tarefa "next"
            // ações após retornar da tarefa "next", se houverem
        }
        userTasks = queue_size((queue_t *)ready_tasks);
    }
    task_exit(0) ; // encerra a tarefa dispatcher
}

// do tipo FCFS
//first come first served
task_t *scheduler_fcfs()
{
    //retorna nulo se nada na fila
    if(ready_tasks != NULL) {
        // lembrando que isso é a fila que eu fiz la no primeiro trabalho
        userTasks--;
        task_t * next = ready_tasks;
        queue_remove((queue_t **)&ready_tasks, (queue_t *)next);    //tira da fila

        next->my_queue = NULL;
        return next;
    } else {
#ifdef DEBUG
        printf("Nada na fila, retornando 0 como proxima tarefa\n");
#endif
        return NULL;
    }
}

//com envelhecimento
task_t *scheduler()
{
    //retorna nulo se nada na fila
    if(ready_tasks != NULL) {
        // lembrando que isso é a fila que eu fiz la no primeiro trabalho
        userTasks--;

        //pegar a task com o melhor prioridade
        int pmin = prio_max+1;
        task_t * next;
        task_t *tsk_tmp = ready_tasks;
        int i;
        for (i = 0; i < queue_size((queue_t *)ready_tasks); i++) {
            if (tsk_tmp->dinamic_prio < pmin) {
                pmin = tsk_tmp->dinamic_prio;
                next = tsk_tmp;
            }
            tsk_tmp = (queue_t *)tsk_tmp->next; // aqui que tem quer ver se ta certo, mas funciona assim
        }

        //retura ela da fila
        queue_remove((queue_t **)&ready_tasks, (queue_t *)next);    //tira da fila

        //envelhece as que sobraram na fila
        tsk_tmp = ready_tasks;
        for (i = 0; i < queue_size((queue_t *)ready_tasks); i++) {
            tsk_tmp->dinamic_prio = tsk_tmp->dinamic_prio + a;
            tsk_tmp = (queue_t *)tsk_tmp->next; // aqui que tem quer ver se ta certo tbm
        }

        // reseta prioridade dinamica da task retirada
        next->dinamic_prio = next->static_prio;
        next->my_queue = NULL;

        return next;
    } else {
#ifdef DEBUG
        printf("Nada na fila, retornando 0 como proxima tarefa\n");
#endif
        return NULL;
    }
}


// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio)
{
    // se nao tem tarefa seta na atual
    if(task == NULL) {
        task = current_tsk;
    }
    if (prio < -20 || prio >20) {
        printf("Prioridade nao pode ser setada, fora do range\n");
    } else {
        task->static_prio = prio;
        task->dinamic_prio = prio;
#ifdef DEBUG
        printf("task_setprio: static_prio de %d : %d\n", task->tid, task->static_prio);
#endif
    }
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task)
{
    // se nao tem tarefa faz na atual
    if(task == NULL) {
        task = current_tsk;
    }
    return task->static_prio;
}

// tratador do sinal, o que fazer quando der um tick
void timer_tratador (int signum)
{

#ifdef DEBUG_QUANTUM
    printf("sigum: %d, quantum: %d, task_id: %d \n", signum, quantum, current_tsk->tid);
#endif

    sysclock_ms++;
    current_tsk->proc_time++;

    if (current_tsk->tid != 1 && !current_tsk->lock) {
        quantum--;
        if (quantum <= 0) {
            userTasks++;
            queue_append((queue_t **) &ready_tasks, (queue_t *) current_tsk);  //colocando na fila para esperar
            current_tsk->my_queue = (queue_t **) &ready_tasks; // atualiza sua variavel de fila
            task_switch(&dispatcher);   //volta pro dispatcher
        }
    }
}

// retorna o relógio atual (em milisegundos)
unsigned int systime ()
{
    return sysclock_ms;
}

// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task)
{
    if (!task) {
        return -1;
    }
    if(task->status == ENDED) {
        return task->exit_code;
    }
    // o lock impede o codigo de ser parado no meio de sua execução
    current_tsk->lock = TRUE;

#ifdef DEBUG
    printf("tarefa %d esperando tarefa %d \n", current_tsk->tid, task->tid);
#endif

    //muda o status da tarefa a ser suspensa
    current_tsk->status = SUSP;
    //fica esperando a outra terminar
    queue_remove((queue_t **)&ready_tasks, (queue_t *)current_tsk);    //tira da fila
    // colocando na fila de tarefas esperando a task terminar
    queue_append((queue_t **) &(task->wait_me_q), (queue_t *) current_tsk);
    // atualiza variavel de fila
    current_tsk->my_queue = (queue_t *) task->wait_me_q;

    current_tsk->lock = FALSE;
    task_switch(&dispatcher);   //volta pro dispatcher


    return task->exit_code;
}

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL
void task_suspend (task_t *task, task_t **queue){
	int current = 0;
	if (!task){
		task = current_tsk;
		current = 1;
	}

	if (task->my_queue){
		queue_remove((queue_t **)&(task->my_queue), (queue_t *)task);    //tira da fila
	}

	queue_append((queue_t **) queue, (queue_t *) task);  //colocando na fila
	task->my_queue = (queue_t **) queue; // atualiza sua variavel de fila

	//se estou suspendendo a mesma que estou executando volta pro dispatcher
	if(current){
		task_switch(&dispatcher);
	}
}

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume (task_t *task){
	if (task->my_queue){
		queue_remove((queue_t **)&(task->my_queue), (queue_t *)task);    //tira da fila
	}
	queue_append((queue_t **) &ready_tasks, (queue_t *) task);  //colocando na fila para esperar
	task->my_queue = (queue_t **) &ready_tasks; // atualiza sua variavel de fila
}


