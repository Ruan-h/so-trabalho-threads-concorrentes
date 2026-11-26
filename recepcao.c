#define _DEFAULT_SOURCE

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

static int fd_demanda = -1;

/* Aguarda o processo filho parar em SIGSTOP apos gravar demanda.txt */
static int aguarda_sigstop(pid_t pid)
{
    int status;
    pid_t r;
    do {
        r = waitpid(pid, &status, WUNTRACED);
    } while (r < 0 && errno == EINTR);

    if (r == pid && WIFSTOPPED(status))
        return 0;

    return -1;
}

/* Leitura rapida de demanda.txt via pread no offset 0 */
static int ler_demanda(void)
{
    char buf[16];
    if (fd_demanda < 0) {
        fd_demanda = open("demanda.txt", O_RDONLY);
        if (fd_demanda < 0)
            return 1;
    }

    ssize_t n = pread(fd_demanda, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        close(fd_demanda);
        fd_demanda = open("demanda.txt", O_RDONLY);
        if (fd_demanda < 0)
            return 1;
        n = pread(fd_demanda, buf, sizeof(buf) - 1, 0);
        if (n <= 0)
            return 1;
    }

    int val = 0;
    for (ssize_t i = 0; i < n && buf[i] >= '0' && buf[i] <= '9'; ++i) {
        val = val * 10 + (buf[i] - '0');
    }
    return (val > 0) ? val : 1;
}

/* Cria o processo cliente via vfork para evitar copia de tabelas de paginas */
static pid_t criar_processo_cliente(void)
{
    pid_t pid = vfork();
    if (pid == 0) {
        execl("./cliente", "cliente", (char *)NULL);
        _exit(1);
    }
    return pid;
}

void thread_recepcao(void *arg)
{
    (void)arg;

    int clientes_criados = 0;

    while (!flag_encerrar &&
           (param_N == 0 || clientes_criados < param_N)) {

        if (fila_tamanho() >= 100) {
            yield_thread();
            continue;
        }

        pid_t pid = criar_processo_cliente();
        if (pid < 0) {
            yield_thread();
            continue;
        }

        if (aguarda_sigstop(pid) != 0) {
            kill(pid, SIGKILL);
            while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {}
            yield_thread();
            continue;
        }

        struct Cliente c;
        c.pid = pid;
        c.tempo_atendimento = ler_demanda();

        c.prioridade = rand() & 1;
        c.tempo_paciencia = (c.prioridade == 1)
                            ? (param_X / 2.0)
                            : (double)param_X;

        clock_gettime(CLOCK_MONOTONIC, &c.hora_chegada);

        if (fila_inserir(&c) != 0) {
            kill(pid, SIGKILL);
            while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {}
            yield_thread();
            continue;
        }

        ++clientes_criados;
        __atomic_add_fetch(&clientes_totais, 1, __ATOMIC_RELAXED);

        yield_thread();
    }

    if (fd_demanda >= 0) {
        close(fd_demanda);
        fd_demanda = -1;
    }
}
