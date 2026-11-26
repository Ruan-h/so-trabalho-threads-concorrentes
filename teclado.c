#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <termios.h>
#include <poll.h>

#include "estruturas.h"

void thread_teclado(void *arg)
{
    (void)arg;

    struct termios orig, raw;
    int is_tty = (tcgetattr(STDIN_FILENO, &orig) == 0);
    if (is_tty) {
        raw = orig;
        /* Desativa modo canonico e echo para leitura imediata sem Enter */
        raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
        raw.c_cc[VMIN]  = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }

    struct pollfd pfd = {
        .fd = STDIN_FILENO,
        .events = POLLIN,
        .revents = 0
    };

    while (!flag_encerrar) {
        /* Timeout 0 para nao bloquear as demais threads no escalonador */
        if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
            char c;
            if (read(STDIN_FILENO, &c, 1) == 1) {
                if (c == 's' || c == 'S')
                    flag_encerrar = 1;
            }
        }

        yield_thread();
    }

    if (is_tty) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig);
    }
}
