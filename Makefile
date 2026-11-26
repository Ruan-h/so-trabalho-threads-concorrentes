CC      = gcc
CFLAGS  = -O3 -march=native -Wall -Wextra -std=c11 -D_GNU_SOURCE
LDFLAGS = -lrt

OBJS_B = recepcao.o teclado.o
OBJS_A = fila.o escalonador.o atendente.o main.o
OBJS_ATENDIMENTO = $(OBJS_A) $(OBJS_B)

.PHONY: all clean

all: analista cliente atendimento

analista: analista.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

cliente: cliente.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

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
	rm -f analista cliente atendimento *.o lng.txt demanda.txt analista_pid.txt
