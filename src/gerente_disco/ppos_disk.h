// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include "ppos_data.h"

#define ATIVO 1
#define DESATIVADO 0
#define SUSPENSO 2

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

typedef struct filaDisk {
  struct filaDisk *next, *prev;
  task_t tarefaGerente;
  int comando;
  int block;
  int *buffer;

} filaDisk_t;

// estrutura que representa um disco no sistema operacional
typedef struct
{
  struct filaDisk_t *filaDisk;
  int sinal;
  int status;
  semaphore_t diskSemaph;
  // completar com os campos necessarios
} disk_t ;

void diskDriverBody(void *args);

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;


#endif
