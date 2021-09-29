// GRR 20182557 Gustavo Valente
#include <assert.h>
#include <stdio.h>
#include "./../includes/queue.h"

int queue_append (queue_t **queue, queue_t *elem) {

    if((*queue) == NULL){
//        printf("DENTRO DO PRIMEIRO IF\n");

        // Head point to first element
        (*queue) = elem;
        (*queue)->next = elem;
        (*queue)->prev = elem;

//        printf("%p e %p e %p\n", *queue, (*queue)->next, (*queue)->prev);
        
        return 0;

    } else {
//        printf("DENTRO DO SEGUDNO IF\n");

        (*queue)->prev->next = elem;
        elem->next = *queue;
        elem->prev = (*queue)->prev;
        (*queue)->prev = elem;

//        printf("TO SAINDO DO SEGUNDO IF\n");
        return 0;
    }

    return -1;
}

void queue_print(char *name, queue_t *queue, void (*print_elem)(void *)) {

    return;
}

int queue_remove (queue_t **queue, queue_t *elem){
    queue_t **auxQueu;
    auxQueu = queue;

    if(*queue == NULL || queue == NULL) {
        printf("SOMETHING GOES WRONG!\n");
        elem = NULL;
    }

    if(*auxQueu == elem) {
        switch(queue_size(*queue)){
            case 1:
                printf("ENTREI NO CASE 1\n");
                elem->prev = NULL;
                elem->next = NULL;
                *queue = NULL;
                break;
            
            default:
                printf("PRIMERO IF\n");
                (*queue)->next->prev = (*queue)->prev;
                (*queue)->prev->next = (*queue)->next;
                (*queue) = (*queue)->next;

                elem->prev = NULL;
                elem->next = NULL;
                break;

        }

        return 0;
    } else if((*auxQueu)->prev == elem) {
        printf("SEGUNDO IF\n");
        (*auxQueu)->prev = (*auxQueu)->prev->prev;
        (*auxQueu)->prev->prev->next = *queue;

        return 0;
    } else{
        printf("TERCEIRO IF\n");
        *auxQueu = elem;
//        while(*auxQueu != elem){
//            printf("ENTREI AQUI CARAI\n");
//            printf("%p e %p\n", auxQueu, elem);
//            *auxQueu = (*auxQueu)->next;
//        } 

        (*auxQueu)->prev->next = (*auxQueu)->next;
        (*auxQueu)->next->prev = (*auxQueu)->prev;

        return 0;
    }

    return -1;
}

int queue_size (queue_t *queue) {
    queue_t *auxQueu = queue;

    if(auxQueu == NULL) return 0;
    else {
//        printf("ENTREI NO CONTADOR DA FILA\n");

        int i = 1;
        while(auxQueu->next != queue){
            auxQueu = auxQueu->next;
            i += 1;
        } 

        printf("SAI DO CONTADOR DA FILA: %d\n", i);
        return i;
    }

    return 0;
}
