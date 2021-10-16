#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "ppos_data.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

static task_t *TarefaAtual, *UltimaTarefa, MainTarefa;
static task_t TarefaDispatcher;
static queue_t *FilaTarefas = NULL;
static int flag = 0;


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
//    printf("Criou a tarefa: %s\n", (char *) arg);
    makecontext (&(task->context), (void*)(*start_routine), 1, arg);
    task->id = flag;
    queue_append (&FilaTarefas, (queue_t *) task);
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
    return -1;
}

// Termina a tarefa corrente
void task_exit (int exit_code){
    #ifdef DEBUG
    printf ("task_exit: tarefa %d sendo encerrada\n", exit_code) ;
    #endif
    if(queue_size(FilaTarefas) > 0)
        task_switch(&TarefaDispatcher);
    else
        task_switch(&MainTarefa);
    return;
}

// Informa o identificador da tarefa corrente
int task_id (){
    #ifdef DEBUG
    printf ("task_id: id da tarefa %d\n", TarefaAtual->id) ;
    #endif
    return TarefaAtual->id;
}

void task_yield () {
    // Caso seja diferente, após a tarefa atual ser executada, a task_yield vai ser chamada
    // E coloca a tarefa que acabou de ser executada no final da fila.
    // Chamando novamente o task_switch para continuar o dispatcher
    if(&MainTarefa != TarefaAtual)
        queue_append((queue_t **) &FilaTarefas, (queue_t *) TarefaAtual);
    task_switch(&TarefaDispatcher);
    return;
}

void task_setprio (task_t *task, int prio) {
//    if(prio < -20) prio = -20;
    task->prioEstatica = prio;
    task->prioDinamica = prio;
}

int task_getprio (task_t *task) {
    if(task == NULL)
        return TarefaAtual->prioEstatica;
    return task->prioEstatica;
}

task_t* scheduler() {
//    task_t *proxima = (task_t *) FilaTarefas;
    task_t *proxima = NULL;
    task_t *FilaAux = (task_t *) FilaTarefas;

    proxima = FilaAux;
    int tam = queue_size(FilaTarefas);
    for(int i = 0; i < tam; i++){
        if(FilaAux->prioDinamica <= proxima->prioDinamica){
            proxima = FilaAux;
        }

        FilaAux = FilaAux->next;
    }

    proxima->prioDinamica = task_getprio(proxima);
    FilaAux = (task_t *) FilaTarefas;
    for(int i = 0; i < tam; i++) {
        FilaAux->prioDinamica -= 1;
        FilaAux = FilaAux->next;
    }

//    task_setprio(proxima, proxima->prioEstatica);
    return proxima;
}

// Quando a tarefa volta para o dispatcher, não começa no começo do while novamente
// Continua de onde a tarefa parou!!!
void Dispatcher () {
    while(FilaTarefas != NULL) {

        task_t *proxima = scheduler();

        if(proxima != NULL) {
            queue_remove(&FilaTarefas, (queue_t *) proxima);
            FilaTarefas = queue_size(FilaTarefas) > 0 ? FilaTarefas : NULL;
            task_switch(proxima);
        }

    }
    task_exit(0);
}

// Inicializa as estruturas internas do SO
void ppos_init() {
    // Salva o contexto da main
    if(flag == 0) {
        getcontext(&MainTarefa.context);
        MainTarefa.id = flag;
        TarefaAtual = &MainTarefa;
        flag += 1;

        task_create(&TarefaDispatcher, Dispatcher, "Dispatcher");
    } else {
        printf("Erro em iniciar o contexto da main\n");
        exit(0);
    }
    // Desabilita o buffer do printf
    setvbuf (stdout, 0, _IONBF, 0);
    return;
}