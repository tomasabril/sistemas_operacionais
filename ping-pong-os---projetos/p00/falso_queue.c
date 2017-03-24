#include "queue.h"
#include<stdio.h>

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem) {
    if (!queue) {printf ("FILA NAO EXISTE!\n"); return;}
    if (!elem) {printf ("ELEMENTO NAO EXISTE!\n"); return;}
    if (elem->prev != NULL && elem->next != NULL) {printf ("ELEMENTO PRESENTE EM OUTRA FILA!\n"); return;}
    if (!(*queue)){
        (*queue) = elem;
        (*queue)->next = elem;
        (*queue)->prev = elem;
    }

    else{
        (*queue)->prev->next = elem;
        elem->prev = (*queue)->prev;
        (*queue)->prev = elem;
        elem->next = (*queue);
        (*queue)->prev = elem;
    }

}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem){
    if (!queue) {printf ("FILA NAO EXISTE!\n"); return NULL;}
    if (!(*queue)) {printf ("FILA ESTA VAZIA!\n"); return NULL;}
    if (!elem) {printf ("ELEMENTO NAO EXISTE!\n"); return NULL;}

    queue_t* aux = (*queue);

    while (elem != aux){
        aux = aux->next;
        if (aux == (*queue)) {printf ("ELEMENTO NAO PERTECENTE A FILA!\n");
            return NULL;}
    }

    if(elem == (*queue) && elem->next == (*queue)){
        (*queue) = NULL;
        elem->next = NULL;
        elem->prev = NULL;

        return elem;
    }
    else if (elem == (*queue) && elem->next != (*queue)){
        (*queue) = elem->next;
        elem->next->prev = elem->prev;
        elem->prev->next = elem->next;
        elem->next = NULL;
        elem->prev = NULL;

        return elem;
    }
    else{
        elem->next->prev = elem->prev;
        elem->prev->next = elem->next;
        elem->next = NULL;
        elem->prev = NULL;

        return elem;
    }
}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue){

    int cont = 0;

    if (!queue) return cont;

    queue_t* aux = queue;

    do{
        cont++;
        aux = aux->next;
    } while(aux != queue);

    return cont;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {

    queue_t* aux = queue;

    if(!queue) {
        printf ("%s: []\n", name);
        return;
    }
    printf ("%s: [", name);

    do{
        print_elem(aux);
        if (aux->next != queue) printf (" ");
        aux = aux->next;
    } while(aux != queue);
    printf ("]\n");
}
