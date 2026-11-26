#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <time.h>
#include "estruturas.h"

/* Definicao das variaveis globais compartilhadas (extern em estruturas.h) */
volatile int flag_encerrar = 0;
volatile int clientes_totais = 0;
volatile int clientes_satisfeitos = 0;
pid_t        analista_pid = 0;
int          param_N = 0;
int          param_X = 0;

/* Prototipos das threads e escalonador */
void thread_recepcao(void *arg);
void thread_teclado(void *arg);
void thread_atendente(void *arg);
void iniciar_escalonador(void);

static int lng_estavazio(sem_t *sem_block) {
    while (sem_wait(sem_block) < 0 && errno == EINTR) {}
    struct stat st;
    int vazio = (stat("lng.txt", &st) != 0 || st.st_size <= 0);
    sem_post(sem_block);
    return vazio;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <N_clientes> <X_paciencia_ms>\n", argv[0]);
        return 1;
    }

    /* Filhos herdam timerslack de 1ns para reduzir latencia do usleep */
    prctl(PR_SET_TIMERSLACK, 1UL, 0UL, 0UL, 0UL);

    param_N = atoi(argv[1]);
    param_X = atoi(argv[2]);
    if (param_N < 0 || param_X <= 0) {
        printf("Uso: %s <N_clientes >= 0> <X_paciencia_ms > 0>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    sem_unlink("/sem_atend");
    sem_unlink("/sem_block");

    sem_t *sem_atend = sem_open("/sem_atend", O_CREAT, 0666, 1);
    sem_t *sem_block = sem_open("/sem_block", O_CREAT, 0666, 1);

    if (sem_atend == SEM_FAILED || sem_block == SEM_FAILED) {
        perror("Erro ao criar semaforos");
        return 1;
    }

    unlink("lng.txt");
    unlink("demanda.txt");
    unlink("analista_pid.txt");

    analista_pid = fork();
    if (analista_pid < 0) {
        perror("Erro ao criar processo analista");
        sem_close(sem_atend);
        sem_close(sem_block);
        sem_unlink("/sem_atend");
        sem_unlink("/sem_block");
        return 1;
    }
    if (analista_pid == 0) {
        execl("./analista", "analista", NULL);
        perror("Erro ao executar ./analista");
        _exit(1);
    }

    /* Aguarda o Analista registrar os handlers e gravar analista_pid.txt */
    for (int i = 0; i < 2000; ++i) {
        if (access("analista_pid.txt", F_OK) == 0) {
            break;
        }
        if (waitpid(analista_pid, NULL, WNOHANG) != 0) {
            break;
        }
        usleep(100);
    }

    struct timespec t_inicio, t_fim;
    clock_gettime(CLOCK_MONOTONIC, &t_inicio);

    registrar_thread(thread_recepcao, NULL);
    if (param_N == 0) {
        registrar_thread(thread_teclado, NULL);
    }
    registrar_thread(thread_atendente, NULL);

    iniciar_escalonador();

    clock_gettime(CLOCK_MONOTONIC, &t_fim);

    /* Descarta clientes que restaram na fila quando N=0 encerra */
    struct Cliente c_restante;
    while (fila_remover(&c_restante) == 0) {
        kill(c_restante.pid, SIGKILL);
        while (waitpid(c_restante.pid, NULL, 0) < 0 && errno == EINTR) {}
        __atomic_sub_fetch(&clientes_totais, 1, __ATOMIC_RELAXED);
    }

    while (!lng_estavazio(sem_block)) {
        if (kill(analista_pid, SIGUSR1) < 0) {
            break;
        }
        usleep(500);
    }

    kill(analista_pid, SIGTERM);
    while (waitpid(analista_pid, NULL, 0) < 0 && errno == EINTR) {}
    unlink("analista_pid.txt");

    double tempo_total_ms = (t_fim.tv_sec - t_inicio.tv_sec) * 1000.0 +
                            (t_fim.tv_nsec - t_inicio.tv_nsec) / 1000000.0;

    double taxa_satisfacao = 0.0;
    if (clientes_totais > 0) {
        taxa_satisfacao = ((double)clientes_satisfeitos / clientes_totais) * 100.0;
    }

    printf("\n====================================\n");
    printf("     RESULTADOS DO ATENDIMENTO      \n");
    printf("====================================\n");
    printf("Clientes Atendidos : %d\n", clientes_totais);
    printf("Clientes Satisfeitos: %d\n", clientes_satisfeitos);
    printf("Taxa de Satisfacao  : %.2f%%\n", taxa_satisfacao);
    printf("Tempo Total Execucao: %.2f ms\n", tempo_total_ms);
    printf("====================================\n");

    sem_close(sem_atend);
    sem_close(sem_block);
    sem_unlink("/sem_atend");
    sem_unlink("/sem_block");

    return 0;
}