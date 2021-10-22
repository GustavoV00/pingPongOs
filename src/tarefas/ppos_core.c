#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "ppos_data.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

static task_t *TarefaAtual, *UltimaTarefa, TarefaMain;
static int flag = 0;

// Inicializa as estruturas internas do SO
void ppos_init() {
    if(flag == 0) {
        getcontext(&TarefaMain.context);
        TarefaMain.id = flag;
        TarefaAtual = &TarefaMain;
        flag += 1;
    }
    
    // Desabilita o buffer do printf
    setvbuf (stdout, 0, _IONBF, 0);
    return;
}

// Cria uma nova tarefa
int task_create (task_t *task, void (*start_routine)(void *),  void *arg){
    char *stack;

    // Salva o contexto atua na váriavel a.
    stack = malloc(STACKSIZE);
    getcontext (&(task->context));
    if(stack){
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }

    makecontext (&(task->context), (void*)(*start_routine), 1, arg);
    task->id = flag;
    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->id) ;
    #endif
    flag += 1;

    return task->id;
}

// Transfere o processador para outra tarefa
int task_switch (task_t *task){
    UltimaTarefa = TarefaAtual;
    TarefaAtual = task;
    #ifdef DEBUG
    printf ("task_switch: trocando contexto %d -> %d\n", UltimaTarefa->id, TarefaAtual->id);
    #endif
    swapcontext (&UltimaTarefa->context, &TarefaAtual->context);
    return task->id;
}

// Termina a tarefa corrente
void task_exit (int exit_code){
    #ifdef DEBUG
    printf ("task_exit: tarefa %d sendo encerrada\n", exit_code) ;
    #endif
    task_switch(&TarefaMain);
    return;
}

// Informa o identificador da tarefa corrente
int task_id (){
    #ifdef DEBUG
    printf ("task_id: id da tarefa %d\n", TarefaAtual->id) ;
    #endif
    return TarefaAtual->id;
}