#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

#include <time.h>
#include <sys/types.h>

/* ── Representação de um cliente na fila ─────────────────────────────────── */
struct Cliente {
    pid_t          pid;
    int            prioridade;       /* 0 = baixa, 1 = alta */
    struct timespec hora_chegada;    /* CLOCK_MONOTONIC no instante do fork */
    int            tempo_atendimento;/* ms, lido de demanda.txt             */
    int            tempo_paciencia;  /* ms: alta → X/2, baixa → X          */
};

/* ── Fila circular (implementada pela dupla) — tipo opaco ───────────────── */
struct FilaCircular;

/* ── API da fila/escalonador (implementada pela dupla) ───────────────────── */
int  fila_inserir(struct Cliente *c); /* 0 = OK, -1 = cheia                 */
int  fila_tamanho(void);             /* número de clientes aguardando       */
void yield_thread(void);             /* cede a CPU de volta ao escalonador  */
void registrar_thread(void (*fn)(void *), void *arg); /* registra thread    */

/* ── Globais compartilhados entre todos os módulos ───────────────────────── */
extern volatile int flag_encerrar;
extern volatile int clientes_totais;
extern volatile int clientes_satisfeitos;
extern pid_t        analista_pid;
extern int          param_N;
extern int          param_X;

#endif /* ESTRUTURAS_H */
