// GRR 20182557 Gustavo Valente
#include <assert.h>
#include <stdio.h>
#include "./../includes/queue.h"

int queue_append (queue_t **queue, queue_t *elem) {

    // Return 0 if sucess
    // Return -1 if not sucess.

    // Verify if the element exists.
    if(elem != NULL){
        // Verify if the element is in the correct queue.
        if(elem->next == NULL && elem->prev == NULL){
            // Insert the first element
            if(queue_size(*queue) == 0){

                // Head point to first element
                (*queue) = elem;
                (*queue)->next = elem;
                (*queue)->prev = elem;


            // Insert any other element
            } else if(queue_size(*queue) > 0) {

                (*queue)->prev->next = elem;
                elem->next = *queue;
                elem->prev = (*queue)->prev;
                (*queue)->prev = elem;

            } else {
                printf("error, fila está negativa \n");
                return -1;
            }

            return 0;
        }else {
            printf("erro, elemento em outra fila\n");
            return -1;
        }
    } else {
        printf("erro, elemento não existe\n");
        return -1;
    }
}

void queue_print(char *name, queue_t *queue, void (*print_elem)(void *)) {
    queue_t *aux = queue;


    printf("%s: [", name);

    // Size of the queue
    int tam = queue_size(queue);
    for(int i = 0; i < tam; i++){
        print_elem(aux);
        aux = aux->next;
        // Just to format the output
        if(i+1 != tam) printf(" ");
    }


    printf("]\n");

    return;
}

int queue_remove (queue_t **queue, queue_t *elem){
    queue_t **aux = queue, *first = *queue;


    // Element need to exists
    if(elem != NULL){
        // Confirm if the queue exists
        if(*queue != NULL && queue != NULL){
            if(*aux == elem) {
                switch(queue_size(*queue)){
                    // Remove the first element if the size is equal to one.
                    case 1:
                        elem->prev = NULL;
                        elem->next = NULL;
                        *queue = NULL;
                        break;

                    // Remove the first element if the size is bigger than one.
                    default:
                        (*queue)->next->prev = (*queue)->prev;
                        (*queue)->prev->next = (*queue)->next;
                        (*queue) = (*queue)->next;

                        elem->prev = NULL;
                        elem->next = NULL;
                        break;

                }

                return 0;
            }
            // Remove the element in the last position
            else if((*aux)->prev == elem) {
                (*aux) = (*aux)->prev;

                (*aux)->prev->next = (*aux)->next;
                (*aux)->next->prev = (*aux)->prev;
                *queue = first;
                elem->next = NULL;
                elem->prev = NULL;

                return 0;
            
            } 
            // Remove the element that is in the middle of the queue
            else{

                while(*aux != elem){
                    *aux = (*aux)->next;

                    // If the element doesn't exist in the queue
                    if(*aux == first){
                        printf("erro: Elemento não pertence a fila.\n");
                        return -1;
                    }
                } 

                queue = aux;
                (*aux)->next->prev = elem->prev;
                (*aux)->prev->next = elem->next;
                *queue = first;
                elem->next = NULL;
                elem->prev = NULL;

                return 0;
            }
        } else {
            printf("erro, a fila não existe ou está vazia\n");
            return -1;
        }
    } else {
        printf("erro, elemento não existe\n");
        return -1;
    }

}

int queue_size (queue_t *queue) {
    queue_t *aux = queue;

    // Size if the queue is empty
    if(aux == NULL) return 0;
    // Count the queue size.
    else {

        int i = 1;
        while(aux->next != queue){
            aux = aux->next;
            i += 1;
        } 

        return i;
    }

    return 0;
}