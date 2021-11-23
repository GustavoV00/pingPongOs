// GRR20182557 Gustavo Valente Nunes
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define PRONTA 0
#define TERMINADA 1

static task_t *TarefaAtual, *UltimaTarefa, MainTarefa;
static task_t TarefaDispatcher;
static queue_t *FilaTarefas = NULL;
static queue_t *FilaAdormecidas = NULL;
static semaphore_t *test = NULL;
static semaphore_t *test2 = NULL;
static struct sigaction action;
static struct itimerval timer;
static int flag = 0;
static int ticks = 0;
static int quantum = 20;
static int lock = 0;
static int block;
static int tarefasUsuario = 0;


// -------------------------------------------------------
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
        tarefasUsuario += 1;

        TarefaAtual = &MainTarefa;
        block = 1;
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
        tarefasUsuario += 1;
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

    if(TarefaAtual->tarefaUsuario == 0 && task_id() != 1){
        TarefaAtual->tarefaUsuario = 1;
        queue_append((queue_t**)&FilaTarefas, (queue_t*)TarefaAtual);
    }

    task_switch(&TarefaDispatcher);
}

// Termina a tarefa corrente
void task_exit (int exit_code){
    TarefaAtual->duracaoDaTarefa = systime();
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", task_id(), TarefaAtual->duracaoDaTarefa, TarefaAtual->tempoNoProcessador, TarefaAtual->ativacoes);
    #ifdef DEBUG
    printf ("task_exit: tarefa %d sendo encerrada\n", exit_code) ;
    #endif

    queue_t *FilaAux;
    while(queue_size((queue_t *) TarefaAtual->tarefasSuspensas) > 0){
        FilaAux = TarefaAtual->tarefasSuspensas;
        queue_remove((queue_t **) &TarefaAtual->tarefasSuspensas, FilaAux);
        queue_append((queue_t **) &FilaTarefas, FilaAux);
    }
        
    if(task_id() != 1){
        TarefaAtual->status = TERMINADA;
        tarefasUsuario -= 1;
        queue_remove(&FilaTarefas, (queue_t *) TarefaAtual);
        task_yield();
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
//    while(tarefasUsuario > 0) {
    while (queue_size(FilaTarefas) > 0 || queue_size(FilaAdormecidas) > 0){
    
        // Caso exista tarefa nas filas de prontas
        if(queue_size(FilaTarefas) > 0){
            task_t *proxima = scheduler();

            if(proxima != NULL) {
                task_switch(proxima);
            }
        }

        // Caso exista tarefas adormecidas
        block = 0;
        if(FilaAdormecidas != NULL){
            task_t *aux = (task_t *) FilaAdormecidas;
            task_t *removido = NULL;

            int tamFila = queue_size((queue_t *) aux);
            for(int i = 0; i < tamFila; i++){
                if(systime() >= aux->deveAcordar && aux != removido) {
                    queue_remove(&FilaAdormecidas, (queue_t *) aux);
                    queue_append(&FilaTarefas, (queue_t *) aux);
                    removido = aux;
                }
            }
        }
        block = 1;
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
    if(sinal == 14 && TarefaAtual->tarefaUsuario == 1 && block == 1){
        quantum -= 1;
        if(quantum == 0 ){ // Quando chega a zero, indica que seu tempo no processador acabou
            quantum = 20;
            task_yield();        
        }
    }
}

int task_join (task_t *task) {
    if(task->status == PRONTA){
        if(task != NULL){
            block = 0;
            // Suspensão das tarefas
            queue_remove(&FilaTarefas, (queue_t *) TarefaAtual);
            queue_append(&task->tarefasSuspensas, (queue_t *) TarefaAtual);
            block = 1;
            task_switch(&TarefaDispatcher);
        }

        return task->id;
    } else 
        return -1;
}

void task_sleep(int t) {
    // Momento em que a tarefa deve acordar em ms
    block = 0;
    TarefaAtual->deveAcordar = t + systime();
    queue_remove(&FilaTarefas, (queue_t *) TarefaAtual);
    queue_append(&FilaAdormecidas, (queue_t *) TarefaAtual);
    block = 1;
    task_switch(&TarefaDispatcher);
}

// ---------------------------------------------------
// Atomic Functions
// ---------------------------------------------------

void enter_cs (int *lock) {
    while (__sync_fetch_and_or (lock, 1)) ; 
}

void leave_cs (int *lock) {
  (*lock) = 0 ;
}

// ---------------------------------------------------
// Semáforos
// ---------------------------------------------------
int sem_create(semaphore_t *s, int value){
    enter_cs(&lock);
    if(!s) return -1;

    s->fila = NULL;
    s->contador = value;
    leave_cs(&lock);
    return 0;
}

int sem_down(semaphore_t *s) {
    enter_cs(&lock);
    if(!s){
        leave_cs(&lock);
        return -1;
    }
    s->contador -= 1;
    test = s;
    if(s->contador < 0) {
        task_t *elem = TarefaAtual;
        block = 0;
        queue_remove(&FilaTarefas, (queue_t *) elem);
        queue_append((queue_t **) &s->fila, (queue_t *) elem);
        leave_cs(&lock);
        task_yield();
        block = 1;
    } else
        leave_cs(&lock);

    return 0;
}

int sem_up(semaphore_t *s) {
    enter_cs(&lock);
    if(!s){
        leave_cs(&lock);
        return -1;
    }

    s->contador += 1;
    test2 = s;
    if(s->contador <= 0){
        if(queue_size((queue_t *) s->fila) > 0){
            block = 0;
            queue_t *elem = (queue_t *) s->fila;
            queue_remove((queue_t **) &s->fila, elem);
            queue_append(&FilaTarefas, elem);
            block = 1;
        }
    }
    leave_cs(&lock);
    return 0;
}

int sem_destroy(semaphore_t *s) {
    enter_cs(&lock);
    if(!s){
        leave_cs(&lock);
        return -1;
    }

    task_t *elem = NULL;
    while(queue_size((queue_t *) s->fila) > 0) {
        elem = s->fila;
        queue_remove((queue_t **) &s->fila, (queue_t *) elem);
        queue_append(&FilaTarefas, (queue_t *) elem);
    }
    leave_cs(&lock);
    return 0;
}


// ---------------------------------------------------
// MQUEUE
// ---------------------------------------------------

// int size é em bytes
// Cria uma fila de mensagens
int mqueue_create(mqueue_t *queue, int max, int size) {
    if(!queue) return -1;

    queue->max_msg = max;
    queue->msg_size = size;
    queue->ultimo = -1;
    queue->primeiro = 0;
    queue->quantidade = 0;
    queue->status = 1;

    // Aloca considerando o tamanho total da fila e o tamanho do tipo
    queue->buffer = malloc(queue->max_msg * queue->msg_size);
    if(!queue->buffer) return -1;

    sem_create(&queue->s_buffer, 1);
    sem_create(&queue->s_vaga, queue->max_msg);
    sem_create(&queue->s_item, 0);

    return 0;
}

// Envia uma mensagem
int mqueue_send(mqueue_t *queue, void *msg) {
    if(!queue || queue->status == 0) return -1;
    sem_down(&queue->s_vaga);
    sem_down(&queue->s_buffer);

    // insere item no buffer
    if(queue->ultimo == queue->max_msg-1) queue->ultimo = -1;
    queue->ultimo += 1;
    memcpy(queue->buffer + (queue->msg_size*queue->ultimo), msg, queue->msg_size);
    queue->quantidade += 1;

    sem_up(&queue->s_buffer);
    sem_up(&queue->s_item);
    return 0;
}

// Recebe uma mensagem
int mqueue_recv(mqueue_t *queue, void *msg) {
    if(!queue || queue->status == 0) return -1;
    sem_down(&queue->s_item);
    sem_down(&queue->s_buffer);

    // Retira item do buffer
    // item = buffer[queue->countCons];
    memcpy(msg, queue->buffer+(queue->msg_size*queue->primeiro), queue->msg_size);
    queue->primeiro += 1;
    queue->quantidade -= 1;
    if(queue->primeiro == queue->max_msg) queue->primeiro = 0;

    sem_up(&queue->s_buffer);
    sem_up(&queue->s_vaga);
    return 0;
}

// Destroy o message queue
int mqueue_destroy(mqueue_t *queue) {
    if(!queue || queue->status == 0) return -1;
    sem_destroy(&queue->s_vaga);
    sem_destroy(&queue->s_item);
    sem_destroy(&queue->s_buffer);
    free(queue->buffer);
    queue->status = 0;
    queue = NULL;
    return 0;
}

// Pega o total de mensagens
int mqueue_msgs(mqueue_t *queue){
    if(queue) return queue->quantidade;
    return -1;
}

