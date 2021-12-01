#include "disk.h"
#include "ppos.h"
#include "ppos_data.h"
#include "ppos_disk.h"
#include "queue.h"
#include <stdio.h>

// AS TAREFAS SUSPENSAS FICAM NA FILA DO DISCO
void diskDriverBody(void *args) {
  while (1) {
    // obtém o semáforo de acesso ao disco
    sem_down(&disk.diskSemaph);

    printf("AAAAAAAAAAAAAAAAAAAAA\n");
    // se foi acordado devido a um sinal do disco
    if (disk.sinal == SIGUSR1) {
      printf("ENTREI AQWUI NO SINAL\n");
      // acorda tarefa cujo pedido foi atendido
      task_t elem = (task_t)disk.filaDisk.tarefaGerente;
      queue_append((queue_t **)&disk.filaDisk, (queue_t *)&disk.filaDisk);
      queue_remove((queue_t **)&FilaTarefas, (queue_t *)&elem);
      // disk.filaDiskSuspensa = NULL;
    }

    // Se o disco estiver livre e houver pedidos de E/S na fila
    if (queue_size((queue_t *)disk.filaDisk) && (disk.filaDisk != NULL)) {
      // escolhe na fila o pedido a ser atendido, usando FCFS
      // solicita ao disco a operação de E/S, usando disk_cmd()
      filaDisk_t *aux = disk.filaDisk;
      disk_cmd(aux->comando, aux->block, aux->buffer);
    }
    sem_up(&disk.diskSemaph);
    TarefaDisk.status = SUSPENSO;

    if (MainTarefa.status == DESATIVADO)
      task_exit(0);

    // Libera o semafaró
    task_yield();
    // Suspende a tarefa correte (Retorna ao dispatcher)
  }
}

int disk_mgr_init(int *numBlocks, int *blockSize) {
  if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0)
    return -1;

  *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
  if (*numBlocks < 0)
    return -1;

  *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
  if (*blockSize < 0)
    return -1;

  if (sem_create(&disk.diskSemaph, 1) < 0)
    return -1;

  disk.filaDisk = NULL;
  disk.sinal = 0;

  task_create(&TarefaDisk, diskDriverBody, "Disk ");
  return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer) {
  sem_down(&disk.diskSemaph);

  filaDisk_t elem;
  elem.block = block;
  elem.buffer = buffer;
  elem.comando = DISK_CMD_READ;

  queue_append((queue_t **)&disk.filaDisk, (queue_t *)&elem);
  // 2 - DORMINDO, 1 - ATIVA, 0 - Morta
  if (TarefaDisk.status == SUSPENSO) {
    TarefaDisk.status = ATIVO;
    printf("Estou entrando dentgro da tarefa suspewnjsa_READ\n");
    //        queue_remove((queue_t **)disk.filaDisk, (queue_t *)&elem);
    queue_append((queue_t **)FilaTarefas, (queue_t *)&TarefaDisk);
    //        TarefaDisk.fila = FilaTarefas;
  }

  TarefaAtual->status = ATIVO;
  sem_up(&disk.diskSemaph);
  task_yield();
  return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer) {
  if (!buffer)
    return -1;
  if (sem_down(&disk.diskSemaph) < 0)
    return -1;

  filaDisk_t elem;
  elem.block = block;
  elem.buffer = buffer;
  elem.comando = DISK_CMD_WRITE;

  queue_append((queue_t **)&disk.filaDisk, (queue_t *)&elem);
  // 2 - DORMINDO, 1 - ATIVA, 0 - Morta
  if (TarefaDisk.status == SUSPENSO) {
    TarefaDisk.status = ATIVO;
    //        queue_remove((queue_t **)disk.filaDisk, (queue_t *)&elem);
    queue_append((queue_t **)FilaTarefas, (queue_t *)&TarefaDisk);
    //        isk.filaDisk = FilaTarefas;
  }

  if (sem_up(&disk.diskSemaph) < 0)
    return -1;

  TarefaAtual = SUSPENSO;
  task_yield();
  return 0;
}
