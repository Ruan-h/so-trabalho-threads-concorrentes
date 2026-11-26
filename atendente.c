#define _POSIX_C_SOURCE 200809L

#include "estruturas.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

static void acionar_analista_e_aguardar(void) {
    if (analista_pid <= 0) return;

    struct stat st;
    if (kill(analista_pid, SIGUSR1) < 0) {
        return;
    }
    int tentativas = 0;
    while (stat("lng.txt", &st) == 0 && st.st_size > 0) {
        usleep(50);
        if (++tentativas % 20 == 0) {
            if (kill(analista_pid, SIGUSR1) < 0) {
                break;
            }
        }
    }
}

void thread_atendente(void *arg) {
    (void)arg;

    sem_t *sem_atend = sem_open("/sem_atend", O_RDWR);
    sem_t *sem_block = sem_open("/sem_block", O_RDWR);
    int fd_lng = open("lng.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    int pendentes_lng = 0;

    while (1) {
        if (flag_encerrar) {
            break;
        }

        struct Cliente c;

        if (fila_remover(&c) != 0) {
            if (param_N > 0 && clientes_totais >= param_N) {
                break;
            }

            yield_thread();
            continue;
        }

        kill(c.pid, SIGCONT);

        while (waitpid(c.pid, NULL, 0) < 0 && errno == EINTR) {}

        struct timespec agora;
        clock_gettime(CLOCK_MONOTONIC, &agora);

        if (sem_atend != SEM_FAILED) {
            while (sem_wait(sem_atend) < 0 && errno == EINTR) {}
            sem_post(sem_atend);
        }

        double chegada_ms = (c.hora_chegada.tv_sec * 1000.0) + (c.hora_chegada.tv_nsec / 1000000.0);
        double agora_ms   = (agora.tv_sec * 1000.0) + (agora.tv_nsec / 1000000.0);
        double tempo_espera = agora_ms - chegada_ms;

        if (tempo_espera <= c.tempo_paciencia) {
            __atomic_add_fetch(&clientes_satisfeitos, 1, __ATOMIC_RELAXED);
        }

        if (sem_block != SEM_FAILED) {
            char pid_buf[32];
            int len = snprintf(pid_buf, sizeof(pid_buf), "%d\n", (int)c.pid);

            while (sem_wait(sem_block) < 0 && errno == EINTR) {}
            if (fd_lng < 0) {
                fd_lng = open("lng.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
            }
            if (fd_lng >= 0 && len > 0) {
                ssize_t w = write(fd_lng, pid_buf, (size_t)len);
                (void)w;
            }
            sem_post(sem_block);
        }

        if (++pendentes_lng >= 10) {
            acionar_analista_e_aguardar();
            pendentes_lng = 0;
        }

        yield_thread();
    }

    if (pendentes_lng > 0) {
        acionar_analista_e_aguardar();
    }

    if (fd_lng >= 0) close(fd_lng);
    if (sem_atend != SEM_FAILED) sem_close(sem_atend);
    if (sem_block != SEM_FAILED) sem_close(sem_block);
}
