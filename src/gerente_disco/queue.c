// GRR20182557 Gustavo Valente Nunes
#include <stdio.h>
#include "queue.h"

int queue_append (queue_t **queue, queue_t *elem) {

    // Verifica se o elemento existe
    if(elem != NULL){
        // Verifica se o elemento ta na fila correta.
        if(elem->next == NULL && elem->prev == NULL){
            // Caso a fila tenha tamanho 0, insere no primeiro elemento.
            if(*queue == NULL){

                // Faz a fila apontar para o primeiro elemento.
                (*queue) = elem;
                (*queue)->next = elem;
                (*queue)->prev = elem;

            // Insere qualquer outro elemento.
            } else {

                // Arruma os ponteiros.
                (*queue)->prev->next = elem;
                elem->next = *queue;
                elem->prev = (*queue)->prev;
                (*queue)->prev = elem;

            } 
            return 0;
        } else 
            printf("Erro: Elemento já está inserido ou está na fila incorreta\n");
    } else 
        printf("Erro: Elemento não existe\n");

    return -1;
}

void queue_print(char *name, queue_t *queue, void (*print_elem)(void *)) {
    queue_t *aux = queue;


    printf("%s: [", name);

    // Pega o tamanho da fila
    int tam = queue_size(queue);
    for(int i = 0; i < tam; i++){
        print_elem(aux);
        aux = aux->next;
        
        // Formata o output, para ficar igual aos testes
        if(i+1 != tam) printf(" ");
    }


    printf("]\n");

    return;
}

int queue_remove (queue_t **queue, queue_t *elem){
    queue_t **aux = queue, *first = *queue;

    // Verifica se o elemento existe
    if(elem != NULL){
        // Confirma que a fila existe 
        if(elem->next != NULL && elem->prev != NULL){

            if(*queue != NULL){
                if(*aux == elem) {
                    switch(queue_size(*queue)){
                        // Remove o primeiro elemento, com a fila de um único elemento
                        case 1:
                            *queue = NULL;
                            break;

                        // Remove o primeiro elemento, para quando a fila tem mais de um elemento
                        default:
                            (*queue)->next->prev = (*queue)->prev;
                            (*queue)->prev->next = (*queue)->next;
                            (*queue) = (*queue)->next;
                            break;

                    }
                }
                // Remove o segundo ou ultimo elemento.
                else {
                    // Verifica se remove o ultimo elemento ou o segundo
                    // Remove o ultimo elemento
                    if((*aux)->prev == elem) 
                        *aux = (*aux)->prev;
                    else {
                        // Remove o segundo elemento
                        // Verifica se o elemento pertece a fila correta.
                        while((*aux = (*aux)->next) != elem){
                            if(*aux == first){
                                printf("Erro: Elemento não pertence a está fila\n");
                                return -1;
                            }
                        }
                    }

                    // Arruma os ponteiros da remoção do ultimo e segundo elemento
                    (*aux)->prev->next = (*aux)->next;
                    (*aux)->next->prev = (*aux)->prev;
                    *queue = first;
                
                } 
                elem->next = NULL;
                elem->prev = NULL;
                return 0;

            } else 
                printf("Erro: Fila não existe\n");
        } else 
            printf("Erro: Elemento não está em nenhuma fila.\n");
    } else 
        printf("Erro: Elemento não existe\n");

    return -1;
}

int queue_size (queue_t *queue) {
    queue_t *aux = queue;

    // Tamanho da fila, se ela estiver vazia
    if(aux == NULL) return 0;
    else {
        int tamanhoFila = 1;
        while((aux = aux->next) != queue) tamanhoFila += 1;
        return tamanhoFila;
    }
}
