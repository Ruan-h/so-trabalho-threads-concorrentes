#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define LOTE     10
#define BUF_SIZE 65536

static volatile sig_atomic_t sinal_usr1 = 0;
static volatile sig_atomic_t sinal_term = 0;
static sem_t *sem_global = SEM_FAILED;
static char buf_estatico[BUF_SIZE];

static void handler_usr1(int sig)
{
    (void)sig;
    sinal_usr1 = 1;
}

static void handler_term(int sig)
{
    (void)sig;
    sinal_term = 1;
}

static void processar_lote(void)
{
    if (sem_global == SEM_FAILED) {
        sem_global = sem_open("/sem_block", O_RDWR);
        if (sem_global == SEM_FAILED)
            return;
    }

    while (sem_wait(sem_global) < 0 && errno == EINTR) {}

    int fd = open("lng.txt", O_RDWR);
    if (fd < 0) {
        sem_post(sem_global);
        return;
    }

    ssize_t lidos;
    do {
        lidos = pread(fd, buf_estatico, sizeof(buf_estatico) - 1, 0);
    } while (lidos < 0 && errno == EINTR);

    if (lidos <= 0) {
        close(fd);
        sem_post(sem_global);
        return;
    }

    int linhas = 0;
    ssize_t corte = 0;
    for (ssize_t i = 0; i < lidos; ++i) {
        if (buf_estatico[i] == '\n') {
            if (++linhas == LOTE) {
                corte = i + 1;
                break;
            }
        }
    }
    if (corte == 0) {
        corte = lidos;
    }

    ssize_t w = write(STDOUT_FILENO, buf_estatico, (size_t)corte);
    (void)w;

    if (corte >= lidos) {
        int r = ftruncate(fd, 0);
        (void)r;
    } else {
        ssize_t restante = lidos - corte;
        ssize_t pw = pwrite(fd, buf_estatico + corte, (size_t)restante, 0);
        (void)pw;
        int r = ftruncate(fd, restante);
        (void)r;
    }

    close(fd);
    sem_post(sem_global);
}

int main(void)
{
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGTERM);
    /* Bloqueia sinais fora de sigsuspend para evitar perda de notificacao */
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    sigdelset(&oldmask, SIGUSR1);
    sigdelset(&oldmask, SIGTERM);

    struct sigaction sa_usr1;
    memset(&sa_usr1, 0, sizeof(sa_usr1));
    sa_usr1.sa_handler = handler_usr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    struct sigaction sa_term;
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = handler_term;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);

    sem_global = sem_open("/sem_block", O_RDWR);

    int pid_fd = open("analista_pid.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (pid_fd >= 0) {
        char pid_buf[32];
        int len = snprintf(pid_buf, sizeof(pid_buf), "%ld\n", (long)getpid());
        if (len > 0) {
            ssize_t pw = write(pid_fd, pid_buf, (size_t)len);
            (void)pw;
        }
        close(pid_fd);
    }

    while (!sinal_term) {
        if (!sinal_usr1) {
            sigsuspend(&oldmask);
        }
        if (sinal_usr1) {
            sinal_usr1 = 0;
            processar_lote();
        }
    }

    if (sem_global != SEM_FAILED) {
        sem_close(sem_global);
    }

    return 0;
}
