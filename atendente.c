#define _POSIX_C_SOURCE 200809L

#include "estruturas.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>

int fila_remover(struct Cliente *out);
int fila_vazia(void);

void thread_atendente(void *arg) {
    (void)arg;

    sem_t *sem_atend = sem_open("/sem_atend", O_RDWR);
    sem_t *sem_block = sem_open("/sem_block", O_RDWR);

    while (1) {
        struct Cliente c;

        if (fila_remover(&c) != 0) {
            // Verifica se a Recepção já terminou e a fila está vazia
            int recepcao_terminou = 0;
            if (param_N > 0 && clientes_totais >= param_N) {
                recepcao_terminou = 1;
            }
            if (flag_encerrar) {
                recepcao_terminou = 1;
            }

            if (recepcao_terminou) {
                break; // Encerra o atendimento
            }

            yield_thread();
            continue;
        }

        // Acorda o processo cliente (parado no SIGSTOP)
        kill(c.pid, SIGCONT);

        // Pequeha pausa para dar tempo do cliente acionar sem_wait
        usleep(1000);

        // Aguarda a finalização do atendimento do cliente via semáforo /sem_atend
        if (sem_atend != SEM_FAILED) {
            sem_wait(sem_atend); // Aguarda o sem_post executado pelo cliente
            sem_post(sem_atend); // Restaura para o próximo cliente
        }

        // Evita processos zumbi
        waitpid(c.pid, NULL, 0);

        // Calcula a satisfação: (hora_atual - hora_chegada) <= paciencia
        struct timespec agora;
        clock_gettime(CLOCK_MONOTONIC, &agora);

        double chegada_ms = (c.hora_chegada.tv_sec * 1000.0) + (c.hora_chegada.tv_nsec / 1000000.0);
        double agora_ms   = (agora.tv_sec * 1000.0) + (agora.tv_nsec / 1000000.0);
        double tempo_espera = agora_ms - chegada_ms;

        if (tempo_espera <= (double)c.tempo_paciencia) {
            __atomic_add_fetch(&clientes_satisfeitos, 1, __ATOMIC_RELAXED);
        }

        //Escreve o PID no ficheiro lng.txt sob proteção do /sem_block
        if (sem_block != SEM_FAILED) {
            sem_wait(sem_block);

            FILE *f = fopen("lng.txt", "a");
            if (f) {
                fprintf(f, "%d\n", c.pid);
                fclose(f);
            }

            sem_post(sem_block);
        }

        // Acorda o processo Analista enviando SIGUSR1
        if (analista_pid > 0) {
            kill(analista_pid, SIGUSR1);
        }

        yield_thread();
    }

    if (sem_atend != SEM_FAILED) sem_close(sem_atend);
    if (sem_block != SEM_FAILED) sem_close(sem_block);
}