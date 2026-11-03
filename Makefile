CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE
LDFLAGS = -lrt

OBJS_B = recepcao.o teclado.o

OBJS_A = fila.o escalonador.o atendente.o main.o

# ── Todos os objetos do executável de atendimento ─────────────────────────
OBJS_ATENDIMENTO = $(OBJS_A) $(OBJS_B)

.PHONY: all clean

all: analista cliente atendimento

# ── Processo Analista (binário independente) ───────────────────────────────
analista: analista.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# ── Processo Cliente (binário do cliente) ──────────────────────────────────
cliente: cliente.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# ── Processo Atendimento (executável principal) ───────────────────────────
atendimento: $(OBJS_ATENDIMENTO)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

recepcao.o: recepcao.c estruturas.h
	$(CC) $(CFLAGS) -c $< -o $@

teclado.o: teclado.c estruturas.h
	$(CC) $(CFLAGS) -c $< -o $@

fila.o: fila.c estruturas.h
	$(CC) $(CFLAGS) -c $< -o $@

escalonador.o: escalonador.c estruturas.h
	$(CC) $(CFLAGS) -c $< -o $@

atendente.o: atendente.c estruturas.h
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c estruturas.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f analista cliente atendimento *.o lng.txt demanda.txt