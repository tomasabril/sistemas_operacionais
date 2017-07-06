// PingPongOS - PingPong Operating System
// Tomas Abril
// 2017_1
//
// interface do driver de disco rígido

#ifndef __DISKDRIVER__
#define __DISKDRIVER__

#include "datatypes.h"

typedef struct pedido_t
{
    struct pedido_t *prev, *next ;	// para usar com a biblioteca de filas (cast)
    int block;
    void *buffer;
    int cmd;
    int atendido;
    task_t *pedinte;
} pedido_t ;

// structura de dados que representa o disco para o SO
typedef struct disk_t
{
    struct pedido_t *fila_pedidos;
    semaphore_t s_disco;
    int npedidos;
    int signal;
} disk_t ;


// inicializacao do driver de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int diskdriver_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer indicado
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer indicado para o disco
int disk_block_write (int block, void *buffer) ;

#endif
