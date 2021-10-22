// GRR20182557 Gustavo Valente Nunes
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos_data.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

static task_t *TarefaAtual, *UltimaTarefa, MainTarefa;
static task_t TarefaDispatcher;
static queue_t *FilaTarefas = NULL;
static struct sigaction action;
static struct itimerval timer;
static int flag = 0;
static int tick = 20;


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
    return task->id;
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

// Indica se volta para a main ou se vai para o dispatcher que é o gerenciador de tarefas. 
void task_yield () {
    // Caso seja diferente, após a tarefa atual ser executada, a task_yield vai ser chamada
    // E coloca a tarefa que acabou de ser executada no final da fila.
    // Chamando novamente o task_switch para continuar o dispatcher
    #ifdef DEBUG
    printf ("Quando a tarefa atual, for a main novamente, significa que chegou no fim.\n") ;
    #endif
    // Condição para o código conseguir acabar. 
    if(&MainTarefa != TarefaAtual)
        queue_append((queue_t **) &FilaTarefas, (queue_t *) TarefaAtual);
    task_switch(&TarefaDispatcher);
    return;
}

void tratamentoDeTicks(int sinal) {
    int auxTick = tick;
    if(flag > 0 && sinal == 14)
        while(auxTick > 0) { auxTick--; }
    task_yield();
}

void task_setprio (task_t *task, int prio) {
    task->prioEstatica = prio;
    task->prioDinamica = prio;
}

int task_getprio (task_t *task) {
    if(task == NULL)
        return TarefaAtual->prioEstatica;
    return task->prioEstatica;
}

// Indica qual vai ser a próxima tarefa. 
task_t* scheduler() {
    task_t *proxima = NULL; // Proxima tarefa
    task_t *FilaAux = (task_t *) FilaTarefas; // FilaxAux é o que vai andar pela Fila

    proxima = FilaAux;
    int tam = queue_size(FilaTarefas);
    // Verifica a tarefa com a menor prioridade
    for(int i = 0; i < tam; i++){
        if(FilaAux->prioDinamica < proxima->prioDinamica){
            proxima = FilaAux;
        }

        FilaAux = FilaAux->next;
    }

    // Faz a FilaAux apontar para o começo da fila novamente
    FilaAux = (task_t *) FilaTarefas;
    // Decrementa 1 da prioridade de todas as tarefas
    for(int i = 0; i < tam; i++) {
        FilaAux->prioDinamica -= 1;
        FilaAux = FilaAux->next;
    }

    task_setprio(proxima, proxima->prioEstatica);
    proxima->prioDinamica -= 1; // Arrumar o output
    queue_remove(&FilaTarefas, (queue_t *) proxima);
    return proxima;
}

// Quando a tarefa volta para o dispatcher, não começa no começo do while novamente
// Continua de onde a tarefa parou!!!
void dispatcher () {
    while(FilaTarefas != NULL) {
        // Seleciona a próxima tarefa
        task_t *proxima = scheduler();

        // Enquanto existir alguma proxima tarefa
        if(proxima != NULL) {
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

        action.sa_handler = tratamentoDeTicks;
        sigemptyset (&action.sa_mask);
        action.sa_flags = 0;
        if (sigaction (SIGALRM, &action, 0) < 0) {
            perror ("Erro em sigaction: ");
            exit (1);
        }
        
        // ajusta valores do temporizador
        timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos
        timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
        timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
        timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos
        
        // arma o temporizador ITIMER_REAL (vide man setitimer)
        if (setitimer (ITIMER_REAL, &timer, 0) < 0) {
            perror ("Erro em setitimer: ");
            exit (1);
        }

        #ifdef DEBUG
        printf ("Salva o contexto da tarefa main e cria a tarefa Dispatcher\n");
        #endif

        task_create(&TarefaDispatcher, dispatcher, "dispatcher");
    } else {
        printf("Erro em iniciar o contexto da main\n");
        exit(0);
    }
    // Desabilita o buffer do printf
    setvbuf (stdout, 0, _IONBF, 0);
    return;
}
