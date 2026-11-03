#include "estruturas.h"
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_THREADS 4
#define STACK_SIZE (64 * 1024)

typedef struct {
    ucontext_t ctx;
    char stack[STACK_SIZE];
    int ativa;
    void (*fn)(void *);
    void *arg;
} ThreadContext;

static ThreadContext threads[MAX_THREADS];
static int num_threads = 0;
static int thread_atual = 0;
static ucontext_t ctx_main;

static void thread_wrapper(void) {
    int idx = thread_atual;
    if (threads[idx].fn) {
        threads[idx].fn(threads[idx].arg);
    }
    threads[idx].ativa = 0;
    yield_thread();
}

void registrar_thread(void (*fn)(void *), void *arg) {
    if (num_threads >= MAX_THREADS) return;

    int idx = num_threads++;
    threads[idx].fn = fn;
    threads[idx].arg = arg;
    threads[idx].ativa = 1;

    getcontext(&threads[idx].ctx);
    threads[idx].ctx.uc_stack.ss_sp = threads[idx].stack;
    threads[idx].ctx.uc_stack.ss_size = STACK_SIZE;
    threads[idx].ctx.uc_link = &ctx_main;
    makecontext(&threads[idx].ctx, thread_wrapper, 0);
}

void yield_thread(void) {
    if (num_threads == 0) return;

    int anterior = thread_atual;
    int proxima = (thread_atual + 1) % num_threads;

    int checados = 0;
    while (!threads[proxima].ativa && checados < num_threads) {
        proxima = (proxima + 1) % num_threads;
        checados++;
    }

    if (checados == num_threads || !threads[proxima].ativa) {
        setcontext(&ctx_main);
        return;
    }

    thread_atual = proxima;
    swapcontext(&threads[anterior].ctx, &threads[proxima].ctx);
}

void iniciar_escalonador(void) {
    if (num_threads == 0) return;
    thread_atual = 0;
    swapcontext(&ctx_main, &threads[0].ctx);
}