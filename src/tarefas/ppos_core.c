#include <stdio.h>
#include <ucontext.h>
#include "ppos_data.h"

// Inicializa as estruturas internas do SO
void ppos_init() {
    // Desabilita o buffer do printf
    setvbuf (stdout, 0, _IONBF, 0);
}

// Cria uma nova tarefa
int task_create (task_t *task, void (*start_routine)(void *),  void *arg){

    return -1;
}

// Transfere o processador para outra tarefa
int task_switch (task_t *task){
    return -1;
}

// Termina a tarefa corrente
void task_exit (int exit_code){
    return;
}

// Informa o identificador da tarefa corrente
int task_id (){
    return -1;
}