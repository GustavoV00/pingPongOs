#include <stdio.h>
#include <stdlib.h>
#include "ppos_data.h"
#include "ppos.h"

task_t p1, p2, p3;
task_t c1, c2, c3;
semaphore_t s_buffer, s_item, s_vaga;
int vagas_ocupadas = 0;
int buffer[5];
int indiceProd = 0;
int indiceCons = 0;

void produtor(void* arg) {
    int item;
    while(1) { 
        task_sleep(1000);
        item = rand() % 100;

        sem_down(&s_vaga);
        sem_down(&s_buffer);

        // insere item no buffer
        vagas_ocupadas += 1;
        buffer[indiceProd] = item;
        indiceProd += 1;
        indiceProd = indiceProd > 5 ? 0 : indiceProd;

        printf("%s %d | vagas: %d\n", (char *) arg, item, vagas_ocupadas);
        sem_up(&s_buffer);
        sem_up(&s_item);
    }
    task_exit(0);
}

void consumidor(void* arg) { 
    int item;
    while(1) { 
        sem_down(&s_item);
        sem_down(&s_buffer);

        // Retira item do buffer
        vagas_ocupadas -= 1;
        item = buffer[indiceCons];
        indiceCons += 1;
        indiceCons = indiceCons > 5 ? 0 : indiceCons;

        sem_up(&s_buffer);
        sem_up(&s_vaga);
        printf("%s %d | vagas: %d\n", (char *) arg, item, vagas_ocupadas);
        task_sleep(1000);
    }
    task_exit(0);
}

int main() {
    printf("main: inicio\n");
    ppos_init();

    // Cria os semafóros
    sem_create(&s_buffer, 1);
    sem_create(&s_item, 0);
    sem_create(&s_vaga, 5);

    task_create(&p1, produtor, "p1 produziu ");
    task_create(&p2, produtor, "p2 produziu ");
    task_create(&p3, produtor, "p3 produziu ");

    task_create(&c1, consumidor, "      c1 consumiu ");
    task_create(&c2, consumidor, "      c2 consumiu ");

    task_join(&p1);
//    task_join(&p2);
//    task_join(&p3);
//    task_join(&c1);
//    task_join(&c2);

    sem_destroy(&s_vaga);
    sem_destroy(&s_item);
    sem_destroy(&s_buffer);

    printf("main: fim\n");
    task_exit(0);
    return 0;
}
