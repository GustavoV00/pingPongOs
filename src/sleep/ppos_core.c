// GRR20182557 Gustavo Valente Nunes
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos_data.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define PRONTA 0
#define TERMINADA 1

static task_t *TarefaAtual, *UltimaTarefa, MainTarefa;
static task_t TarefaDispatcher;
static queue_t *FilaTarefas = NULL;
static queue_t *FilaAdormecidas = NULL;
static struct sigaction action;
static struct itimerval timer;
static int flag = 0;
static int ticks = 0;
static int quantum = 20;


// Protótipo das funções
// -------------------------------------------------------
unsigned int systime();
int task_create (task_t *task, void (*start_routine)(void *),  void *arg);
int task_switch (task_t *task);
int task_id ();
void task_yield();
void task_sleep(int t);
int task_join (task_t *task);
void task_exit (int exit_code);
void task_ticks(int sinal);
void task_setprio (task_t *task, int prio);
int task_getprio (task_t *task);
task_t* scheduler();
void dispatcher();
void ppos_init();

// -------------------------------------------------------

// Inicializa as estruturas internas do SO
void ppos_init() {
    // Salva o contexto da main
    if(flag == 0) {
//        getcontext(&MainTarefa.context);
        MainTarefa.id = flag;
        MainTarefa.tarefaUsuario = 1;
        MainTarefa.duracaoDaTarefa = 0;
        MainTarefa.tempoNoProcessador = 0;
        MainTarefa.tarefasSuspensas = NULL;
        MainTarefa.status = PRONTA;

        TarefaAtual = &MainTarefa;
        queue_append (&FilaTarefas, (queue_t *) &MainTarefa);
//        printf("Inserindo na fila a tarefa: %d | ppos_init\n", MainTarefa.id);
        flag += 1;

        #ifdef DEBUG
        printf ("Salva o contexto da tarefa main e cria a tarefa Dispatcher\n");
        #endif

        action.sa_handler = task_ticks;
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

        task_create(&TarefaDispatcher, dispatcher, "dispatcher");
    } else {
        printf("Erro em iniciar o contexto da main\n");
        exit(0);
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

    task->id = flag;
    task->duracaoDaTarefa = 0;
    task->tempoNoProcessador = 0;
    task->tarefasSuspensas = NULL;
    task->tarefaUsuario = 1;
    task->status = PRONTA;
    if(task->id == 1)
        task->tarefaUsuario = 0;

    if(task->tarefaUsuario == 1){
//        printf("Inserindo na fila a tarefa: %d | task_create\n", task->id);
        queue_append (&FilaTarefas, (queue_t *) task);
    }

    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->id) ;
    #endif
    flag += 1;

    makecontext (&(task->context), (void*)(*start_routine), 1, arg);
    return task->id;
}

// Transfere o processador para outra tarefa
int task_switch (task_t *task){
    UltimaTarefa = TarefaAtual;
    TarefaAtual = task;
    #ifdef DEBUG
    printf ("task_switch: trocando contexto %d -> %d\n", UltimaTarefa->id, TarefaAtual->id);
    #endif
    TarefaAtual->ativacoes += 1;
    swapcontext (&UltimaTarefa->context, &TarefaAtual->context);
    return task->id;
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

    task_switch(&TarefaDispatcher);
    return;
}

// Termina a tarefa corrente
void task_exit (int exit_code){
    TarefaAtual->duracaoDaTarefa = systime();
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", task_id(), TarefaAtual->duracaoDaTarefa, TarefaAtual->tempoNoProcessador, TarefaAtual->ativacoes);
    #ifdef DEBUG
    printf ("task_exit: tarefa %d sendo encerrada\n", exit_code) ;
    #endif

    queue_t *FilaAux = TarefaAtual->tarefasSuspensas;
    while(queue_size((queue_t *) TarefaAtual->tarefasSuspensas) > 0){
        queue_remove((queue_t **) &TarefaAtual->tarefasSuspensas, FilaAux);
        queue_append((queue_t **) &FilaTarefas, FilaAux);
        FilaAux = FilaAux->next;
    }
        
    if(TarefaAtual->tarefaUsuario == 1){
        TarefaAtual->status = TERMINADA;
        queue_remove(&FilaTarefas, (queue_t *) TarefaAtual);
        task_switch(&TarefaDispatcher); 
    }
}

task_t* scheduler() {
    task_t *proxima = NULL; // Proxima tarefa
    task_t *FilaAux = (task_t *) FilaTarefas; // FilaxAux é o que vai andar pela Fila

    proxima = FilaAux;
    FilaAux = FilaAux->next;
    int tam = queue_size(FilaTarefas);
    // Verifica a tarefa com a menor prioridade
    for(int i = 0; i < tam; i++){
        if(FilaAux->prioDinamica <= proxima->prioDinamica){
            proxima = FilaAux;
        }
        FilaAux = FilaAux->next;
    }

    FilaAux = (task_t *) FilaTarefas; // FilaxAux é o que vai andar pela Fila
    for(int i = 0; i < tam; i++){
        FilaAux->prioDinamica -= 1;
        if(FilaAux->prioDinamica < -20)
            FilaAux->prioDinamica = -20;
        FilaAux = FilaAux->next;
    }

    task_setprio(proxima, proxima->prioEstatica);
    proxima->prioDinamica -= 1;
    return proxima;
}

// Quando a tarefa volta para o dispatcher, não começa no começo do while novamente
// Continua de onde a tarefa parou!!!
void dispatcher () {
    // As tarefas adormecidas e as tarefas que estão nas filas precisãm ser terminadas. 
    while(queue_size(FilaTarefas) > 0 || queue_size(FilaAdormecidas) > 0) {
    
        // Caso exista tarefa nas filas de prontas
        if(queue_size(FilaTarefas) > 0){
            task_t *proxima = scheduler();

            if(proxima != NULL) {
                task_switch(proxima);
            }
        }


        // Caso exista tarefas adormecidas
        if(queue_size(FilaAdormecidas) > 0){
            task_t *aux = (task_t *) FilaAdormecidas;
            task_t *removido = NULL;

            int tamFila = queue_size((queue_t *) aux);
            for(int i = 0; i < tamFila; i++){
                if(systime() >= aux->deveAcordar && aux != removido) {
//                    printf("Elemento que deve acordr: %d\n", elem->id);
                    queue_remove(&FilaAdormecidas, (queue_t *) aux);
                    queue_append(&FilaTarefas, (queue_t *) aux);
                    removido = aux;
                }
                aux = aux->next;    
            }
        }
    }
    task_exit(0);
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

unsigned int systime() {
    return ticks;
}

// Realiza o tratamento de ticks
void task_ticks(int sinal) {
    ticks += 1;
    TarefaAtual->tempoNoProcessador += 1;
    if(sinal == 14 && TarefaAtual->tarefaUsuario == 1){
        quantum -= 1;
        if(quantum == 0){ // Quando chega a zero, indica que seu tempo no processador acabou
            quantum = 20;
            task_yield();        
        }
    }
}

int task_join (task_t *task) {
    if(task->status == PRONTA){
        if(task != NULL){

            // Suspensão das tarefas
            queue_remove(&FilaTarefas, (queue_t *) TarefaAtual);
            queue_append(&task->tarefasSuspensas, (queue_t *) TarefaAtual);
            task_switch(&TarefaDispatcher);
        }

        return task->id;
    } else 
        return -1;
}

void task_sleep(int t) {
    // Momento em que a tarefa deve acordar em ms
    TarefaAtual->deveAcordar = t + systime();
    queue_remove(&FilaTarefas, (queue_t *) TarefaAtual);
    queue_append(&FilaAdormecidas, (queue_t *) TarefaAtual);
    task_switch(&TarefaDispatcher);
}

