#define _POSIX_C_SOURCE 200809L

#include "estruturas.h"
#include <string.h>

#define MAX_FILA 100

static struct Cliente fila[MAX_FILA];
static int num_elementos = 0;

/* Calcula o deadline absoluto do cliente em milissegundos */
static double calcular_deadline(const struct Cliente *c) {
    double chegada_ms = (c->hora_chegada.tv_sec * 1000.0) + 
                        (c->hora_chegada.tv_nsec / 1000000.0);
    return chegada_ms + c->tempo_paciencia;
}

int fila_tamanho(void) {
    return num_elementos;
}

int fila_vazia(void) {
    return num_elementos == 0;
}

/* Inserção ordenada por Earliest Deadline First (EDF) */
int fila_inserir(struct Cliente *c) {
    if (num_elementos >= MAX_FILA) {
        return -1; // Fila cheia
    }

    double novo_deadline = calcular_deadline(c);
    int i = num_elementos - 1;

    // Desloca elementos com deadline maior para o final
    while (i >= 0 && calcular_deadline(&fila[i]) > novo_deadline) {
        fila[i + 1] = fila[i];
        i--;
    }

    fila[i + 1] = *c;
    num_elementos++;
    return 0;
}

/* Remove e retorna o cliente com a deadline mais próxima */
int fila_remover(struct Cliente *out) {
    if (num_elementos == 0) {
        return -1; // Fila vazia
    }

    *out = fila[0];
    for (int i = 0; i < num_elementos - 1; i++) {
        fila[i] = fila[i + 1];
    }
    num_elementos--;
    return 0;
}