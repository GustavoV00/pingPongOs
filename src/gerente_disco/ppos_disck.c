#include <stdio.h>
#include "disk.h"
#include "ppos.h"
#include "ppos_disk.h"
#include "queue.h"
#include "ppos_data.h"

#define ATIVO 1
#define DESATIVADO 0
#define SUSPENSO 2

static disk_t disk;

// AS TAREFAS SUSPENSAS FICAM NA FILA DO DISCO
void diskDriverBody(void *args){
    while(1) {
        // obtém o semáforo de acesso ao disco
        sem_down(&disk.diskSemaph);

        // se foi acordado devido a um sinal do disco
        if(disk.sinal > 0){
            // acorda tarefa cujo pedido foi atendido
            disk.status = ATIVO;
            task_t *elem = (task_t *) disk.filaDisk;
            queue_remove((queue_t **) disk.filaDisk, (queue_t *) elem);
            queue_append((queue_t **) FilaTarefas, (queue_t *) elem);
        }
    
        // Se o disco estiver livre e houver pedidos de E/S na fila
        if(queue_size((queue_t *) disk.filaDisk) && (disk.filaDisk != NULL)) {
             // escolhe na fila o pedido a ser atendido, usando FCFS
             // solicita ao disco a operação de E/S, usando disk_cmd()
            disk_cmd(disk.comando, disk.block, disk.buffer);
        }
        disk.status = SUSPENSO;

        // Libera o semafaró
        sem_up(&disk.diskSemaph);
        task_yield();
        // Suspende a tarefa correte (Retorna ao dispatcher)
    }
}

int disk_mgr_init (int *numBlocks, int *blockSize){
    if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0) return -1;

    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if(*numBlocks < 0) return -1;

    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if(*blockSize < 0) return -1;
    
    if(sem_create(&disk.diskSemaph, 1) < 0) return -1;
    
    disk.filaDisk = NULL;
    disk.sinal = 0;
    disk.status = ATIVO; // Ativa
    disk.buffer = NULL;

    task_create(&disk.tarefaGerente, diskDriverBody, "Disco ");
    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer){
    if(!buffer) return -1;
    if(sem_down(&disk.diskSemaph) < 0) return -1;

    disk.block = block;
    disk.buffer = buffer;
    disk.tarefaGerente = *TarefaAtual;
    disk.comando = DISK_CMD_READ;
    task_t elem = disk.tarefaGerente;

    queue_append((queue_t **) &disk.filaDisk, (queue_t *) &elem);
    // 2 - DORMINDO, 1 - ATIVA, 0 - Morta
    if(disk.status == SUSPENSO) {
        disk.status = ATIVO;
        task_t elem = disk.tarefaGerente;
        queue_remove((queue_t **) disk.filaDisk, (queue_t *) &elem);
        queue_append((queue_t **) FilaTarefas, (queue_t *) &elem);
    }

    if(sem_up(&disk.diskSemaph) < 0) return -1;
    task_yield();
    return -1;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer){
    if(!buffer) return -1;
    if(sem_down(&disk.diskSemaph) < 0) return -1;

    disk.block = block;
    disk.buffer = buffer;
    disk.tarefaGerente = *TarefaAtual;
    disk.comando = DISK_CMD_WRITE;
    task_t elem = disk.tarefaGerente;

    queue_append((queue_t **) &disk.filaDisk, (queue_t *) &elem);
    // 2 - DORMINDO, 1 - ATIVA, 0 - Morta
    if(disk.status == SUSPENSO) {
        disk.status = ATIVO;
        task_t elem = disk.tarefaGerente;
        queue_remove((queue_t **) disk.filaDisk, (queue_t *) &elem);
        queue_append((queue_t **) FilaTarefas, (queue_t *) &elem);
    }


    if(sem_up(&disk.diskSemaph) < 0) return -1;
    task_yield();
    return 0;
}
