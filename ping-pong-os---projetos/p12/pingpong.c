// PingPongOS - PingPong Operating System
// Tomas Abril
// 2017_1


#include "queue.h"
#include "pingpong.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>

#include <signal.h>
#include <sys/time.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"



#define STACKSIZE 32768		/* tamanho de pilha das threads */ //copiei do p01, nao sei o porque desse tamanho em especifico
#define TRUE 1
#define FALSE 0

// #define DEBUG true
// #define DEBUG_JOIN true
// #define DEBUG_QUANTUM
// #define DEBUG_SLEEP
// #define DEBUGSLEEP2
#define DEBUG_SEMAF
// #define DEBUG_BARR
#define DEBUG_MSG


int id = 0;             //vai somando pra cada tarefa ter id diferente
task_t main_tsk;        // main task
task_t *current_tsk;    // task sendo executada no momento

task_t dispatcher;      //task que controla os despachamentos
task_t *ready_tasks = NULL; //lista de tarefas prontas
task_t *sleep_tasks = NULL; //lista de tarefas dormindo
int userTasks = 0;      //contador de tarefas

int a = -1;         //aging coeficient
int prio_min = -20; //essa é a task que executara antes
int prio_max = 20;  //ultima coisa a ser executada
int tick_u = 1000;
int quantum_max = 20;
int quantum = 0;    //essa variavel vai decrescendo para cada tarefa
unsigned int sysclock_ms = 0;
int preemp = TRUE;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;
// estrutura de inicialização to timer
struct itimerval timer;

// minhas funções
task_t *scheduler_fcfs();
task_t *scheduler();
void dispatcher_body(void *arg);
void timer_tratador(int signum);
void init_main();
void init_temporizador();


// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init ()
{
    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf (stdout, 0, _IONBF, 0);

    init_main();
    task_create(&dispatcher, dispatcher_body, "dispatcher ");
    init_temporizador();
}

void init_main(){
    //------------ criando main ---------------------------------------------
    main_tsk.tid = id++;	//comeca com id 0
    main_tsk.init_time = 0;
    main_tsk.proc_time = 0;
    main_tsk.exec_time = 0;
    main_tsk.activations = 1;
    main_tsk.status = EXEC;
    main_tsk.wait_me_q = NULL;
    main_tsk.lock = FALSE;
    main_tsk.wake_error = 0;
    userTasks++;
    task_setprio(&main_tsk, 0);
    current_tsk = &main_tsk;

#ifdef DEBUG
    printf("inicializando main com id %d \n", (&main_tsk)->tid);
#endif
}

void init_temporizador(){
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
            task->exit_code = -10;
            task->status = READY;
            task->wait_me_q = NULL;
            task->lock = FALSE;
            task->wake_error = 0;

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
#if defined(DEBUG) || defined(DEBUGSLEEP2)
    printf("task_exit: tarefa %d sendo encerrado com codigo %d\n", current_tsk->tid, exitCode);
#endif

    unsigned int now = systime();
    unsigned int exec_time = now - current_tsk->init_time;
    current_tsk->exec_time = exec_time;

    printf("Task %d exit: exec_time %d ms, proc_time %d ms, %d activations\n", current_tsk->tid, exec_time, current_tsk->proc_time, current_tsk->activations);

    current_tsk->exit_code = exitCode;
    current_tsk->status = ENDED;

#if defined(DEBUG) || defined(DEBUG_JOIN) || defined(DEGUB_SLEEP) || defined(DEBUGSLEEP2)
    printf("exit: tarefa %d encerrada com status = ", current_tsk->tid);
    if(current_tsk->status == READY) {
        printf("READY");
    } else if(current_tsk->status == EXEC) {
        printf("EXEC");
    } else if(current_tsk->status == SUSP) {
        printf("SUSP");
    } else if(current_tsk->status == ENDED) {
        printf("ENDED");
    }
    else {printf("ERRO de status");}
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
        queue_append((queue_t **) &ready_tasks, (queue_t *) current_tsk);  //colocando na fila para esperar
        current_tsk->my_queue = (queue_t **) &ready_tasks; // atualiza sua variavel de fila
        current_tsk->status = READY;
    }

    task_switch(&dispatcher);   //volta pro dispatcher
}

void dispatcher_body(void *arg)
{
    while ( userTasks > 0 ) {
        // vendo se precisa acordar alguma tarefa--------------------------
#if defined(DEBUG) || defined(DEBUG_SLEEP)
    printf("\nvamos ver se tem algo pra acordar\n sleep queue size %d\n", queue_size((queue_t *)sleep_tasks));
#endif
        if(queue_size((queue_t *)sleep_tasks) > 0) {
            task_t * nextslp;
            task_t *tsk_tmp = sleep_tasks;
            int i;
            int tamanho_fila = queue_size((queue_t *)sleep_tasks);
            for (i = 0; i < tamanho_fila; i++) {
                nextslp = tsk_tmp;
                tsk_tmp = (queue_t *)tsk_tmp->next; // aqui que tem quer ver se ta certo, mas funciona assim
                if((int)systime() >= nextslp->waketime){
#if defined(DEBUG) || defined(DEBUG_SLEEP)
    printf("systime: %d \ntask_id: %d \n wakeat: %d\n", (int)systime(), nextslp->tid, nextslp->waketime);
#endif
                    //retura ela da fila de dormindo
                    queue_remove((queue_t **)&sleep_tasks, (queue_t *)nextslp);
                    nextslp->status = READY;
                    // colocando na fila de tarefas a executar
#if defined(DEBUG) || defined(DEBUG_SLEEP)
    printf("\ntirei da fila de sleep, colocando na fila prontas\n");
#endif
                    queue_append((queue_t **) &ready_tasks, (queue_t *) nextslp);
                    // atualiza variavel de fila
                    nextslp->my_queue = (queue_t **) &ready_tasks;
                    nextslp->waketime = 0;
                 }
             }
        }
        //-----------------------------------------------------------------

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

    }
    task_exit(0) ; // encerra a tarefa dispatcher
}


//com envelhecimento
task_t *scheduler()
{
    //retorna nulo se nada na fila
    if(ready_tasks != NULL) {
        // lembrando que isso é a fila que eu fiz la no primeiro trabalho

        //pegar a task com a melhor prioridade
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
        next->status = EXEC;

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

    // tarefa não será preemptada se ela for o dispatcher ou se lock estiver ativo
    if (current_tsk->tid != 1 && !current_tsk->lock && preemp) {
        quantum--;
        if (quantum <= 0) {
            queue_append((queue_t **) &ready_tasks, (queue_t *) current_tsk);  //colocando na fila para esperar
            current_tsk->status = READY;
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

#if defined(DEBUG) || defined(DEBUG_JOIN) || defined(DEGUB_SLEEP) || defined(DEBUGSLEEP2)
    printf("join: status da tarefa atual = ");
    if(current_tsk->status == READY) {
        printf("READY");
    } else if(current_tsk->status == EXEC) {
        printf("EXEC");
    } else if(current_tsk->status == SUSP) {
        printf("SUSP");
    } else if(current_tsk->status == ENDED) {
        printf("ENDED");
    }
    else {printf("ERRO de status");}
    printf("\n");
#endif
#if defined(DEBUG) || defined(DEBUG_JOIN) || defined(DEGUB_SLEEP) || defined(DEBUGSLEEP2)
    printf("join: status da tarefa que vou dar join = ");
    if(task->status == EXEC) {
        printf("EXEC");
    } else if(task->status == READY) {
        printf("READY");
    } else if(task->status == SUSP) {
        printf("SUSP");
    } else if(task->status == ENDED) {
        printf("ENDED");
    }
    else {printf("ERRO de status");}
    printf("\n");
#endif

    if(task->status == ENDED) {
#if defined(DEBUG) || defined(DEBUG_JOIN) ||defined(DEBUG_SLEEP) || defined(DEBUGSLEEP2)
    printf("join: %d ia esperar mas tarefa %d ja acabou \n", current_tsk->tid, task->tid);
#endif
        return task->exit_code;
    }
    // o lock impede o codigo de ser parado no meio de sua execução
    current_tsk->lock = TRUE;

#if defined(DEBUG) || defined(DEBUG_JOIN) ||defined(DEBUG_SLEEP) || defined(DEBUGSLEEP2)
    printf("tarefa %d esperando tarefa %d \n", current_tsk->tid, task->tid);
#endif

    //muda o status da tarefa a ser suspensa
    current_tsk->status = SUSP;
    //fica esperando a outra terminar

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
    printf("task suspend --------- \n");
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
    printf("task resume --------- \n");
	if (task->my_queue){
		queue_remove((queue_t **)&(task->my_queue), (queue_t *)task);    //tira da fila
	}
	queue_append((queue_t **) &ready_tasks, (queue_t *) task);  //colocando na fila para esperar
	task->my_queue = (queue_t **) &ready_tasks; // atualiza sua variavel de fila
}

// suspende a tarefa corrente por t segundos
void task_sleep (int t){
    t = t*1000;
    // o lock impede o codigo de ser parado no meio de sua execução
    current_tsk->lock = TRUE;

#if defined(DEBUG) || defined(DEBUG_SLEEP)
    printf("tarefa %d foi dormir por %d milisegundos\n", current_tsk->tid, t);
#endif

    //muda o status da tarefa a ser suspensa
    current_tsk->status = SLEEP;
    // tarefa está executando, portanto não está em nenhuma fila
    //queue_remove((queue_t **)&ready_tasks, (queue_t *)current_tsk);//
    // colocando na fila de tarefas dormindo
    queue_append((queue_t **) &sleep_tasks, (queue_t *) current_tsk);
    // atualiza variavel de fila
    current_tsk->my_queue = (queue_t **) &sleep_tasks;
    current_tsk->waketime = systime() + t;

    current_tsk->lock = FALSE;
    task_switch(&dispatcher);   //volta pro dispatcher
}

// semáforos-----------------------------------------------------
// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value){
    //se nao tem semaforo retorna erro
    if(!s){
        return -1;
    }
    // o lock impede o codigo de ser parado no meio de sua execução
    current_tsk->lock = TRUE;

    s->cont = value;
    s->fila = NULL;

#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("sem_create: novo semaforo, valor: %d\n", value);
#endif

    current_tsk->lock = FALSE;
    return 0;
}

// requisita o semáforo
int sem_down (semaphore_t *s){
    //se nao tem semaforo retorna erro
    if(s == NULL){
        return -1;
    }
    // o lock impede o codigo de ser parado no meio de sua execução
    current_tsk->lock = TRUE;

    int ok = 0;
    s->cont--;

#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("sem_down: contador: %d \n", s->cont);
#endif

    if (s->cont < 0){
        //faltou recurso, vamos suspender
#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("sem_down: suspendendo tarefa %d \n", current_tsk->tid);
#endif

        current_tsk->status = SUSP;
        queue_append((queue_t **) &(s->fila), (queue_t *) current_tsk);
        current_tsk->my_queue = (queue_t *) s->fila;
        task_switch(&dispatcher);
#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("acordando, oi sou a tarefa %d na funcao sem_down. wake_error = %d status = ", current_tsk->tid, current_tsk->wake_error);
    if(current_tsk->status == READY) {
        printf("READY");
    } else if(current_tsk->status == EXEC) {
        printf("EXEC");
    } else if(current_tsk->status == SUSP) {
        printf("SUSP");
    } else if(current_tsk->status == ENDED) {
        printf("ENDED");
    }
    else {printf("ERRO de status");}
    printf("\n");
#endif
        if(current_tsk->wake_error == -1) {
            current_tsk->wake_error = 0;
            printf("depois do if\n");
            // o semaforo foi destruido antes dessa tarefa voltar
#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("sem_down: semaforo foi destruido antes da tarefa %d voltar\n", current_tsk->tid);
#endif
            ok = -1;
            current_tsk->status = READY;
        }
    }

    current_tsk->lock = FALSE;
    return ok;
}

// libera o semáforo
int sem_up (semaphore_t *s){
    //se nao tem semaforo retorna erro
    if(!s){
        return -1;
    }
    // o lock impede o codigo de ser parado no meio de sua execução
    current_tsk->lock = TRUE;

    s->cont++;
#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("sem_up: tarefa %d deu up. valor: %d\n", current_tsk->tid, s->cont);
#endif

    if(s->fila){
        //se tem gente esperando vamos acordar o primeiro
        s->fila->status = READY;
        s->fila->my_queue = NULL;
        task_t * acordada = queue_remove((queue_t **)&(s->fila), (queue_t *)s->fila);
        queue_append ((queue_t **) &ready_tasks, (queue_t *) acordada);
        acordada->my_queue = (queue_t **) &ready_tasks;
#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("sem_up: acordando tarefa %d \n", acordada->tid);
#endif
    }

    current_tsk->lock = FALSE;
    return 0;
}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s){
    //se nao tem semaforo retorna erro
    if(!s){
        return -1;
    }
    // o lock impede o codigo de ser parado no meio de sua execução
    current_tsk->lock = TRUE;

#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("sem_destroy: contador: %d \n", s->cont);
#endif

    while (s->fila) {
        s->fila->my_queue = NULL;
        task_t * acordada = queue_remove((queue_t **)&(s->fila), (queue_t *)s->fila);
        queue_append ((queue_t **) &ready_tasks, (queue_t *) acordada);
        acordada->my_queue = (queue_t **) &ready_tasks;
        acordada->wake_error = -1;
#if defined(DEBUG) || defined(DEBUG_SEMAF)
    printf("sem_destroy: acordando tarefa %d \n", acordada->tid);
#endif
    }
    s->cont = 0;
    s->fila = NULL;
    s = NULL;

    current_tsk->lock = FALSE;
    return 0;
}

// barreiras ----------------------------------------------------------

// Inicializa uma barreira para N tarefas
int barrier_create (barrier_t *b, int N){
    if (!b){
        return -1;
    }
    current_tsk->lock = TRUE;

    b->cont = 0;
    b->n_max = N;
    b->fila = NULL;
#if defined(DEBUG) || defined(DEBUG_BARR)
    printf("barrier_create: barreira criada para %d tarefas \n", N);
#endif

    current_tsk->lock= FALSE;
    return 0 ;
}

// Chega a uma barreira
int barrier_join (barrier_t *b){
    if (!b){
        return -1;
    }
    current_tsk->lock = TRUE;
    int ok = 0;

    // se tem espaco coloca pra esperar na barreira
    b->cont++;
    if(b->cont < b->n_max){
        current_tsk->status = SUSP;
        queue_append((queue_t **) &(b->fila), (queue_t *) current_tsk);
        current_tsk->my_queue = (queue_t *) b->fila;
#if defined(DEBUG) || defined(DEBUG_BARR)
    printf("barrier_join: suspendendo tarefa %d \n", current_tsk->tid);
#endif
        // current_tsk->lock = FALSE;
        task_switch(&dispatcher);
        if(current_tsk->status == SUSP){
            // a barreira foi destruida antes dessa tarefa voltar
            ok = -1;
            current_tsk->status = READY;
        }
    }
    else{
        // se encheu acorda todas as tarefas da fila
        while (b->fila) {
            b->fila->my_queue = NULL;
            task_t * acordada = queue_remove((queue_t **)&(b->fila), (queue_t *)b->fila);
            queue_append ((queue_t **) &ready_tasks, (queue_t *) acordada);
            acordada->my_queue = (queue_t **) &ready_tasks;
            acordada->status = READY;
#if defined(DEBUG) || defined(DEBUG_BARR)
    printf("barrier_join: acordando tarefa %d \n", acordada->tid);
#endif
        }
        b->cont = 0;
    }

    current_tsk->lock = FALSE;
    return ok;
}

// Destrói uma barreira
int barrier_destroy (barrier_t *b){
    if (!b){
        return -1;
    }
    current_tsk->lock= TRUE;
#if defined(DEBUG) || defined(DEBUG_BARR)
printf("barrier_destroy: destruindo barreira com %d tarefas esperando \n", b->cont);
#endif

    while (b->fila) {
        b->fila->my_queue = NULL;
        task_t * acordada = queue_remove((queue_t **)&(b->fila), (queue_t *)b->fila);
        queue_append ((queue_t **) &ready_tasks, (queue_t *) acordada);
        acordada->my_queue = (queue_t **) &ready_tasks;
        acordada->wake_error = -1;

#if defined(DEBUG) || defined(DEBUG_BARR)
printf("barrier_destroy: acordando tarefa %d \n", acordada->tid);
#endif
    }
    b->cont = 0;

    current_tsk->lock = FALSE;
    return 0;
}

// filas de mensagens  --------------------------------------------------

// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size){
    if(!queue){
        return -1;
    }
    current_tsk->lock = TRUE;
#if defined(DEBUG) || defined(DEBUG_MSG)
printf("mqueue_create: criando fila de mensagens \n");
#endif

    queue->fila_size = max;
    queue->msg_size = size;
    queue->read_indice = 0;
    queue->write_indice = 0;
    queue->fila = malloc(max*size +1);

    sem_create(&queue->s_fila, 1);
    sem_create(&queue->s_vaga, max);
    sem_create(&queue->s_ocupado, 0);

    current_tsk->lock = FALSE;
    return 0;
}

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg){
    if(!queue){
        return -1;
    }
    current_tsk->lock = TRUE;

    if(sem_down(&queue->s_vaga) < 0){
        current_tsk->lock = FALSE;
        return -1;
    }
    if(sem_down(&queue->s_fila) < 0){
        current_tsk->lock = FALSE;
        return -1;
    }

#if defined(DEBUG) || defined(DEBUG_MSG)
printf("mqueue_send: colocando mensagem da fila \n");
#endif
    //copiando mensagem
    memcpy(&(queue->fila[queue->write_indice * queue->msg_size]), msg, queue->msg_size);
    queue->write_indice = (queue->write_indice + 1) % queue->fila_size;

    sem_up(&queue->s_ocupado);
    sem_up(&queue->s_fila);

    current_tsk->lock = FALSE;
    return 0;
}

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg){
    if(!queue){
        return -1;
    }
    current_tsk->lock = TRUE;

    if(sem_down(&queue->s_ocupado) <0){
        current_tsk->lock = FALSE;
        return -1;
    }
    if(sem_down(&queue->s_fila) <0){
        current_tsk->lock = FALSE;
        return -1;
    }

#if defined(DEBUG) || defined(DEBUG_MSG)
printf("mqueue_recv: tirando mensagem da fila \n");
#endif
    //copiando mensagem
    memcpy(msg, &(queue->fila[queue->read_indice * queue->msg_size]), queue->msg_size);
    queue->read_indice = (queue->read_indice + 1) % queue->fila_size;

    sem_up(&queue->s_vaga);
    sem_up(&queue->s_fila);

    current_tsk->lock = FALSE;
    return 0;
}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue){
    if(!queue){
        return -1;
    }
    current_tsk->lock = TRUE;
#if defined(DEBUG) || defined(DEBUG_MSG)
printf("mqueue_destroy: apagando fila \n");
#endif

    // limpando memoria
    memset(queue->fila, 0, (queue->fila_size)*(queue->msg_size) +1);
    queue->fila_size = 0;
    queue->msg_size = 0;
    queue->read_indice = 0;
    queue->write_indice = 0;

    sem_destroy(&queue->s_fila);
    sem_destroy(&queue->s_vaga);
    sem_destroy(&queue->s_ocupado);

    free(queue->fila);
    queue = NULL;

    current_tsk->lock = FALSE;
    return 0;
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue){
    if(!queue){
        return -1;
    }
    if((&queue->s_ocupado)->cont >= 0){
#if defined(DEBUG) || defined(DEBUG_MSG)
printf("mqueue_msgs: %d mensagens na fila \n", (&queue->s_ocupado)->cont);
#endif
        return (&queue->s_ocupado)->cont;
    }else{
        return -1;
    }
}
