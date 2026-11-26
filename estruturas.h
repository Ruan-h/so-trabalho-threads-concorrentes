#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

#include <time.h>
#include <sys/types.h>

struct Cliente {
    pid_t           pid;
    int             prioridade;        /* 0 = baixa, 1 = alta */
    struct timespec hora_chegada;      /* CLOCK_MONOTONIC ao entrar na fila */
    int             tempo_atendimento; /* lido de demanda.txt */
    double          tempo_paciencia;   /* ms: alta = X/2.0, baixa = X */
};

struct FilaCircular;

/* API da fila e do escalonador */
int  fila_inserir(struct Cliente *c);
int  fila_remover(struct Cliente *out);
int  fila_tamanho(void);
int  fila_vazia(void);
void yield_thread(void);
void registrar_thread(void (*fn)(void *), void *arg);

/* Variaveis globais compartilhadas */
extern volatile int flag_encerrar;
extern volatile int clientes_totais;
extern volatile int clientes_satisfeitos;
extern pid_t        analista_pid;
extern int          param_N;
extern int          param_X;

#endif /* ESTRUTURAS_H */
