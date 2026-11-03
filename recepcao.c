#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "estruturas.h"

/* Aguarda até 2 s pelo filho parar (SIGSTOP) em intervalos de 1 ms */
static int aguarda_sigstop(pid_t pid)
{
    int status;
    for (int tentativas = 0; tentativas < 2000; ++tentativas) {
        pid_t r = waitpid(pid, &status, WNOHANG | WUNTRACED);
        if (r == pid && WIFSTOPPED(status))
            return 0;
        /* 1 ms entre tentativas — tempo suficiente para o filho rodar */
        struct timespec ts = {0, 1000000L};
        nanosleep(&ts, NULL);
    }
    return -1; /* timeout */
}

/* Lê o inteiro de demanda.txt via I/O de baixo nível */
static int ler_demanda(void)
{
    char buf[32];
    int fd = open("demanda.txt", O_RDONLY);
    if (fd < 0)
        return 1; /* fallback seguro */

    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0)
        return 1;

    buf[n] = '\0';
    return atoi(buf);
}

void thread_recepcao(void *arg)
{
    (void)arg;

    int  clientes_criados = 0;
    char *const argv_cliente[] = {"./cliente", NULL};

    while (!flag_encerrar &&
           (param_N == 0 || clientes_criados < param_N)) {

        /* Bloqueia criação quando a fila está cheia */
        if (fila_tamanho() >= 100) {
            yield_thread();
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            /* fork falhou — aguarda um ciclo e tenta de novo */
            yield_thread();
            continue;
        }

        if (pid == 0) {
            /* ── filho ── */
            execv("./cliente", argv_cliente);
            /* Se execv retornou, algo deu errado */
            _exit(1);
        }

        /* ── pai (thread Recepção) ── */

        /* Registra hora_chegada IMEDIATAMENTE após o fork */
        struct Cliente c;
        memset(&c, 0, sizeof(c));
        c.pid = pid;
        clock_gettime(CLOCK_MONOTONIC, &c.hora_chegada);

        /* Espera o filho dar SIGSTOP (ele escreve demanda.txt antes disso) */
        if (aguarda_sigstop(pid) != 0) {
            /* Filho não parou — mata e ignora */
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            yield_thread();
            continue;
        }

        c.tempo_atendimento = ler_demanda();

        /* Sorteia prioridade: 0 = baixa, 1 = alta */
        c.prioridade = rand() % 2;
        c.tempo_paciencia = (c.prioridade == 1)
                            ? param_X / 2
                            : param_X;

        if (fila_inserir(&c) != 0) {
            /* Fila ainda lotada entre a checagem e o insert (race window) */
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            yield_thread();
            continue;
        }

        ++clientes_criados;
        __atomic_add_fetch(&clientes_totais, 1, __ATOMIC_RELAXED);

        yield_thread();
    }
}
