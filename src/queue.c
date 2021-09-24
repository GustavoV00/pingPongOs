// GRR 20182557 Gustavo Valente
#include <assert.h>
#include <stdio.h>
#include "./../includes/queue.h"

int queue_append (queue_t **queue, queue_t *elem) {


    printf("TESTEA\n");
    if((void **) queue == NULL){
        printf("DENTRO DO IF\n");
        (*queue)->next = elem;
        (*queue)->prev = elem;
        return 0;
    }

    return -1;
}

void queue_print(char *name, queue_t *queue, void (*print_elem)(void *)) {

    return;
}

int queue_remove (queue_t **queue, queue_t *elem){
    return 0;
}

int queue_size (queue_t *queue) {

    if(queue == NULL){
        printf("ENTREI AQUI\n");
        return 0;
    } 
    else {
        int i;
        for(i = 0; queue->next != queue->prev; i++) queue->next++;
        printf("TESTES: %d\n", i);
        return i;
    }
    return 0;
}
