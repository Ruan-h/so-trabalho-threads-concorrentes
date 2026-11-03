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

#define MAX_PIDS      4096
#define PID_BUF_LEN   24    /* dígitos de um pid_t + '\n' + margem */
#define LOTE          10    /* PIDs processados por sinal           */

/* ── Manipulador de SIGUSR1 ─────────────────────────────────────────────── */
static void handler_usr1(int sig)
{
    (void)sig;

    sem_t *sem = sem_open("/sem_block", O_RDWR);
    if (sem == SEM_FAILED)
        return;

    sem_wait(sem);

    /* Abre lng.txt para leitura */
    int fd = open("lng.txt", O_RDONLY);
    if (fd < 0) {
        sem_post(sem);
        sem_close(sem);
        return;
    }

    /* Determina tamanho do arquivo */
    off_t tamanho = lseek(fd, 0, SEEK_END);
    if (tamanho <= 0) {
        close(fd);
        sem_post(sem);
        sem_close(sem);
        return;
    }
    lseek(fd, 0, SEEK_SET);

    /* Lê todo o arquivo em memória */
    char *conteudo = malloc((size_t)tamanho + 1);
    if (!conteudo) {
        close(fd);
        sem_post(sem);
        sem_close(sem);
        return;
    }

    ssize_t lidos = read(fd, conteudo, (size_t)tamanho);
    close(fd);

    if (lidos <= 0) {
        free(conteudo);
        sem_post(sem);
        sem_close(sem);
        return;
    }
    conteudo[lidos] = '\0';

    /* Divide em linhas, imprime as primeiras LOTE e descarta-as */
    char *linhas[MAX_PIDS];
    int   total = 0;

    char *ptr = conteudo;
    while (*ptr && total < MAX_PIDS) {
        linhas[total++] = ptr;
        char *nl = strchr(ptr, '\n');
        if (!nl) break;
        *nl = '\0';
        ptr = nl + 1;
    }

    int imprimir = (total < LOTE) ? total : LOTE;
    for (int i = 0; i < imprimir; ++i)
        dprintf(STDOUT_FILENO, "%s\n", linhas[i]);

    /* Reescreve o arquivo sem as primeiras LOTE linhas */
    fd = open("lng.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        for (int i = imprimir; i < total; ++i) {
            dprintf(fd, "%s\n", linhas[i]);
        }
        close(fd);
    }

    free(conteudo);
    sem_post(sem);
    sem_close(sem);
}

/* ── main ─────────────────────────────────────────────────────────────────── */
int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler_usr1;
    sigemptyset(&sa.sa_mask);
    /* SA_RESTART garante que pause() reinicie se um outro sinal chegar */
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    for (;;)
        pause();

    return 0;
}
