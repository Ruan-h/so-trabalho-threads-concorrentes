#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include "estruturas.h"

/* Definição das variáveis globais compartilhadas (extern em estruturas.h) */
volatile int flag_encerrar = 0;
volatile int clientes_totais = 0;
volatile int clientes_satisfeitos = 0;
pid_t        analista_pid = 0;
int          param_N = 0;
int          param_X = 0;

/* Protótipos das threads e escalonador */
void thread_recepcao(void *arg);
void thread_teclado(void *arg);
void thread_atendente(void *arg);
void iniciar_escalonador(void);

/* Verifica se o ficheiro lng.txt já está completamente limpo */
static int lng_estavazio(sem_t *sem_block) {
    sem_wait(sem_block);
    int vazio = 1;
    FILE *f = fopen("lng.txt", "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        if (ftell(f) > 0) {
            vazio = 0;
        }
        fclose(f);
    }
    sem_post(sem_block);
    return vazio;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <N_clientes> <X_paciencia_ms>\n", argv[0]);
        return 1;
    }

    param_N = atoi(argv[1]);
    param_X = atoi(argv[2]);

    srand(time(NULL));

    // Limpeza de semáforos antigos
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

    // Cria o Processo Analista[cite: 1, 2]
    analista_pid = fork();
    if (analista_pid == 0) {
        execl("./analista", "analista", NULL);
        perror("Erro ao executar ./analista");
        _exit(1);
    }

    struct timespec t_inicio, t_fim;
    clock_gettime(CLOCK_MONOTONIC, &t_inicio);

    // Registra as threads no escalonador ULT
    registrar_thread(thread_recepcao, NULL);
    if (param_N == 0) {
        registrar_thread(thread_teclado, NULL); // Monitora a tecla 's'[cite: 1]
    }
    registrar_thread(thread_atendente, NULL);

    // Inicia a execução concorrente
    iniciar_escalonador();

    clock_gettime(CLOCK_MONOTONIC, &t_fim);

    // Regra Obrigatória para N=0: Esvaziar completamente o lng.txt antes de encerrar[cite: 1]
    while (!lng_estavazio(sem_block)) {
        kill(analista_pid, SIGUSR1);
        usleep(50000); // 50ms para o Analista processar[cite: 2]
    }

    // Finaliza o processo Analista
    kill(analista_pid, SIGTERM);
    waitpid(analista_pid, NULL, 0);

    // Impressão dos Resultados Competitivos[cite: 1, 2]
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