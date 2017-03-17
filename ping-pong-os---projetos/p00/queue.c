#include "queue.h"
#include <stdio.h>


//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem){
    if(queue != NULL ){
        if(elem != NULL){
            if(elem->prev || elem->next){
                // inicializando lista nula
                if(*queue == NULL){
                    *queue = elem;
                    elem->prev = elem;
                    elem->next = elem;
                }
                // inserindo no fim da lista
                else{
                    elem->next = *queue;
                    elem->prev = (*queue)->prev;
                    ((*queue)->prev)->next = elem;
                    (*queue)->prev = elem;
                }

            } else printf("\nelemento esta em outra lista");
        }else printf("\nelemento nao existe");
    }else printf("\nfila nao existe");

}
//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem) {

}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) {

}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {

}

// so pra compilar
//void main(){}

