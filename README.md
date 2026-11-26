# Trabalho 1 - Sistemas Operacionais (UFC)

Implementação de um simulador de atendimento concorrente com troca de contexto em nível de usuário (`ucontext.h`), gerenciamento de processos clientes e análise assíncrona de dados.

---

## Estrutura dos Módulos

- **`fila.c`**: Implementação da fila de prioridade utilizando o algoritmo **EDF (*Earliest Deadline First*)** com desempate pelo menor tempo de atendimento, organizando os clientes com base no tempo limite de paciência ($\text{hora\_chegada} + \text{tempo\_paciencia}$) para maximizar a taxa de satisfação.
- **`escalonador.c`**: Gerenciador de threads em nível de usuário (*User-Level Threads*) utilizando a API `<ucontext.h>`, realizando a troca cooperativa de contexto sem o uso de bibliotecas externas como `pthread`.
- **`atendente.c`**: Thread responsável por remover clientes da fila EDF, acionar a retomada do processo cliente via sinal `SIGCONT`, sincronizar a conclusão do atendimento com o semáforo POSIX `/sem_atend`, calcular as estatísticas de satisfação, gravar os PIDs em lote no arquivo `lng.txt` sob proteção de `/sem_block` e notificar o Analista enviando `SIGUSR1`.
- **`main.c`**: Ponto de entrada do sistema. Define as variáveis globais compartilhadas, inicializa os semáforos POSIX, dispara o processo Analista via `fork()`/`execl()`, aguarda a inicialização via `analista_pid.txt`, inicia o escalonador ULT e exibe o relatório de desempenho ao final.
- **`recepcao.c`**: Thread responsável pela criação dos processos clientes via `vfork()` e `execl()`, sorteio de prioridades (50% Alta / 50% Baixa), leitura da demanda de atendimento via `pread()` e inserção na fila (respeitando o limite de 100 clientes em espera).
- **`analista.c`**: Processo independente que grava seu PID em `analista_pid.txt` e aguarda sinais `SIGUSR1` com `sigsuspend()`. Ao acordar, realiza a leitura e remoção em lote de até 10 PIDs do arquivo `lng.txt` utilizando o semáforo `/sem_block`.
- **`teclado.c`**: Thread de monitoramento não-bloqueante da entrada padrão (`STDIN_FILENO`) via `termios` e `poll()` para captura da tecla `'s'` (encerramento quando $N=0$).
- **`estruturas.h`**: Interface comum do sistema contendo as definições de structs, protótipos da API da fila/escalonador e declarações `extern` de variáveis globais compartilhadas.
- **`cliente.c`**: Código fornecido pelo professor para a simulação do processo cliente.
- **`Makefile`**: Script de compilação dos módulos e binários.

---

## Compilação

Para compilar o projeto completo:

```bash
make
```

### Executáveis Gerados
- **`atendimento`**: Binário principal que integra o escalonador ULT, a fila EDF e as threads de Recepção, Atendimento e Teclado.
- **`analista`**: Binário do processo assíncrono de análise de PIDs.
- **`cliente`**: Binário da aplicação cliente fornecida.

### Limpeza de Artefatos

```bash
make clean
```

---

## Execução

```bash
./atendimento <N_clientes> <X_paciencia_ms>
```

- **`N_clientes`**: Quantidade total de clientes a serem gerados ($0$ para modo contínuo/infinito).
- **`X_paciencia_ms`**: Tempo máximo de paciência (em milissegundos) para clientes de prioridade Baixa (clientes de prioridade Alta possuem paciência $X/2$ ms).

### Exemplos de Uso

1. **Quantidade Determinada de Clientes ($N > 0$):**
   ```bash
   ./atendimento 20 500
   ```

2. **Modo Contínuo ($N = 0$):**
   ```bash
   ./atendimento 0 500
   ```
   Pressione `s` ou `S` no terminal para encerrar o atendimento e drenar o arquivo `lng.txt`.
