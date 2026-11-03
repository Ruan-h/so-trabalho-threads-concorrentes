# Trabalho 1 - Sistemas Operacionais (UFC)

Implementação de simulador de atendimento concorrente com troca de contexto em nível de usuário (`ucontext.h`), processos clientes e análise assíncrona.

## Estrutura dos Módulos

- `estruturas.h`: Definição de structs, variáveis globais e assinaturas de funções para integração entre os módulos.
- `recepcao.c`: Thread responsável pela criação de clientes via `fork()`/`execv()`, sorteio de prioridade e inserção na fila de atendimento (respeitando o limite de 100 clientes).
- `analista.c`: Processo independente responsável por ler em lotes de até 10 PIDs do arquivo `lng.txt` sob sincronização via semáforo POSIX `/sem_block`.
- `teclado.c`: Thread de leitura não-bloqueante no terminal via `termios` e `select()` para captura da tecla `'s'` (encerramento quando $N=0$).
- `Makefile`: Regras de compilação dos módulos e do executável `analista`.

## Compilação

Para compilar os módulos e o processo analista:

```bash
make
```

Isso gerará:
- `analista` (executável do processo analista)
- `recepcao.o` (objeto para linkagem com o escalonador)
- `teclado.o` (objeto para linkagem com o escalonador)

Para limpar os artefatos de compilação:

```bash
make clean
```

## Integração com o Escalonador e Atendente

Para a linkagem final com o escalonador e a fila, o módulo principal deve definir as variáveis globais e implementar as funções declaradas em `estruturas.h`:

- Funções necessárias:
  - `int fila_inserir(struct Cliente *c);`
  - `int fila_tamanho(void);`
  - `void yield_thread(void);`
  - `void registrar_thread(void (*fn)(void *), void *arg);`

- Variáveis globais necessárias:
  - `volatile int flag_encerrar;`
  - `volatile int clientes_totais;`
  - `volatile int clientes_satisfeitos;`
  - `pid_t analista_pid;`
  - `int param_N;`
  - `int param_X;`
