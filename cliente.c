#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>

int main(void)
{
    int tempo;
    srand((unsigned)time(NULL) ^ (getpid() << 16));
    int x = rand() % 10;
    if (x == 0)
        tempo = 15;
    else if (x > 0 && x <= 3)
        tempo = 5;
    else
        tempo = 1;

    FILE *demanda = fopen("demanda.txt", "w+");
    if (demanda) {
        fprintf(demanda, "%d", tempo);
        fclose(demanda);
    }

    raise(SIGSTOP);

    sem_t *sem = sem_open("/sem_atend", O_RDWR);
    if (sem != SEM_FAILED)
        sem_wait(sem);

    usleep(tempo * 1000);

    if (sem != SEM_FAILED) {
        sem_post(sem);
        sem_close(sem);
    }

    return 0;
}
