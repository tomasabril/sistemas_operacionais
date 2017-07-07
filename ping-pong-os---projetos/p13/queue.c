// --- Tomas Abril --- //
// 2017
//
// Biblioteca de filas

#include "queue.h"
#include <stdio.h>


//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem)
{
    if(queue != NULL ) {
        if(elem != NULL) {
            if(elem->prev == NULL && elem->next == NULL) {
                // inicializando lista nula
                if(*queue == NULL) {
                    *queue = elem;
                    elem->prev = elem;
                    elem->next = elem;
                }
                // inserindo no fim da lista
                else {
                    elem->next = *queue;
                    elem->prev = (*queue)->prev;
                    ((*queue)->prev)->next = elem;
                    (*queue)->prev = elem;
                }

            } else printf("\nappend: elemento esta em outra lista\n");
        } else printf("\nappend: elemento nao existe\n");
    } else printf("\nappend: fila nao existe\n");

}
//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
    if(queue != NULL) {
        if(*queue != NULL) {
            if(elem != NULL) {
                if(elem->next != NULL && elem->prev != NULL) {
                    queue_t *elemtmp = *queue;
                    while (elemtmp != elem) {
                        elemtmp = elemtmp->next;
                        if(elemtmp == *queue) {
                            printf("\nremove: elemento nao esta na fila\n");
                            return NULL;
                        }
                    }
                    // retirando o elemento finalmente, meus deus quanto if
                    // caso unico elemento na fila
                    if(elem == *queue && elem->next == elem) {
                        *queue = NULL;
                        elem->next = NULL;
                        elem->prev = NULL;
                    }
                    // caso primeiro da fila
                    else if(elem == *queue) {
                        *queue = elem->next;
                        (elem->prev)->next = elem->next;
                        (elem->next)->prev = elem->prev;
                        elem->prev = NULL;
                        elem->next = NULL;
                    }
                    // outros casos
                    else {
                        (elem->prev)->next = elem->next;
                        (elem->next)->prev = elem->prev;
                        elem->prev = NULL;
                        elem->next = NULL;
                    }
                    return elem;
                }
            } else printf("\nremove: elemento nao existe\n");
        } else printf("\nremove: fila vazia\n");
    } else printf("\nremove: fila nao existe\n");
    return NULL;
}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue)
{
    int i = 0;
    if(queue != NULL) { //fila existe
        i++;
        queue_t *elemtmp = queue;
        while(elemtmp->next != queue) {
            i++;
            elemtmp = elemtmp->next;
        }
    }
    return i;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
    if(queue != NULL) { //fila existe
        printf("[");
        queue_t *elemtmp = queue;
        while(elemtmp->next != queue) {
            print_elem(elemtmp);
            elemtmp = elemtmp->next;
            if(elemtmp->next != queue) {
                printf(" ");
            }
        }
        printf("]\n");
    } else {
        printf("[]");
    }
}
