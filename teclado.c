#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>

#include "estruturas.h"

void thread_teclado(void *arg)
{
    (void)arg;

    /* Salva configuração original do terminal */
    struct termios orig, raw;
    tcgetattr(STDIN_FILENO, &orig);

    raw = orig;
    /* Desativa modo canônico e echo: leitura caractere a caractere, sem eco */
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    while (!flag_encerrar) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        /* Timeout de 50 ms para não bloquear o escalonador por muito tempo */
        struct timeval tv = {0, 50000};

        int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
            char c;
            if (read(STDIN_FILENO, &c, 1) == 1) {
                if (c == 's' || c == 'S')
                    flag_encerrar = 1;
            }
        }

        yield_thread();
    }

    /* Restaura terminal antes de sair */
    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
}
