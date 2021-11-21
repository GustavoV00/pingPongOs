// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   int prioEstatica;        // Prioridade Estática
   int prioDinamica;        // Prioridade Dinâmica 
   int tarefaUsuario;       // Indica se a tarefa pertence ao usuário
   int duracaoDaTarefa;     // Tempo de duração da tarefa
   int ativacoes;           // Indica quantas vezes essa tarefa foi ativada. (task_switch)
   int tempoNoProcessador;  // Tempo em que a tarefa teve no processador. 
   int deveAcordar;
   int status;
   queue_t *tarefasSuspensas;
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  struct task_t *fila;
  int contador;
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  void *buffer;
  int ultimo;
  int primeiro;
  int max_msg;
  int msg_size;
  int quantidade;
  int status;

  semaphore_t s_buffer;
  semaphore_t s_vaga;
  semaphore_t s_item;
  
  // preencher quando necessário
} mqueue_t ;

#endif
