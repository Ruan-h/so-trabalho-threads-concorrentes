CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDFLAGS = -lrt

# ── Objetos linkáveis pelos colegas de equipe ──────────────────────────────
OBJS = recepcao.o teclado.o

.PHONY: all clean

all: analista cliente $(OBJS)

# ── Processo Analista (binário independente) ───────────────────────────────
analista: analista.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# ── Processo Cliente (binário do cliente) ──────────────────────────────────
cliente: cliente.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# ── Módulos compilados como objetos para link posterior ───────────────────
recepcao.o: recepcao.c estruturas.h
	$(CC) $(CFLAGS) -c $< -o $@

teclado.o: teclado.c estruturas.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f analista cliente $(OBJS)
