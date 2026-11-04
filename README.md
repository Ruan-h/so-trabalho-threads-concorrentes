# Trabalho 1 - Sistemas Operacionais (UFC)

Implementação de um simulador de atendimento concorrente com troca de contexto em nível de usuário (`ucontext.h`), gerenciamento de processos clientes e análise assíncrona de dados.

---

## 🛠️ Estrutura dos Módulos

- **`fila.c` / `fila.h`**: Implementação da fila de prioridade utilizando o algoritmo **EDF (*Earliest Deadline First*)**, que organiza os clientes com base no tempo limite de paciência ($\text{hora\_chegada} + \text{tempo\_paciencia}$) para maximizar a taxa de satisfação.
- **`escalonador.c`**: Gerenciador de threads em nível de usuário (*User-Level Threads*) utilizando a API `<ucontext.h>`, realizando a troca cooperativa de contexto sem o uso de bibliotecas externas como `pthread`.
- **`atendente.c`**: Thread responsável por remover clientes da fila EDF, acionar a retomada do processo cliente via sinal `SIGCONT`, sincronizar a conclusão do atendimento com o semáforo POSIX `/sem_atend`, calcular as estatísticas de satisfação, gravar os PIDs no arquivo `lng.txt` sob proteção de `/sem_block` e notificar o Analista enviando `SIGUSR1`.
- **`main.c`**: Ponto de entrada do sistema. Define as variáveis globais compartilhadas, inicializa os semáforos POSIX, dispara o processo Analista via `fork()`/`execv()`, inicia o escalonador ULT e exibe o relatório de desempenho ao final.

- **`recepcao.c`**: Thread responsável pela criação dos processos clientes via `fork()` e `execv()`, sorteio estocástico de prioridades (50% Alta / 50% Baixa), leitura da demanda de atendimento e inserção na fila (respeitando o limite de 100 clientes em espera).
- **`analista.c`**: Processo independente que aguarda sinais `SIGUSR1`. Ao acordar, realiza a leitura e remoção em lote de até 10 PIDs do arquivo `lng.txt` de forma segura utilizando o semáforo `/sem_block`.
- **`teclado.c`**: Thread de monitoramento não-bloqueante da entrada padrão (`STDIN_FILENO`) via `termios` e `select()` para captura da tecla `'s'` (utilizada para encerramento gracioso quando $N=0$).
- **`estruturas.h`**: Interface comum do sistema contendo as definições de structs, protótipos da API da fila/escalonador e declarações `extern` de variáveis globais compartilhadas.

### Arquivos Auxiliares
- **`cliente.c`**: Código fornecido pelo professor para a simulação do processo cliente (gera o arquivo `demanda.txt`, faz a pausa com `SIGSTOP` e executa o atendimento).
- **`Makefile`**: Script de compilação automatizada de todos os módulos e binários.

---

## ⚙️ Compilação

Para compilar o projeto completo, execute o comando abaixo no terminal:

```bash
make
```

### Artefatos Gerados

#### Executáveis
- **`atendimento`**: Binário principal que integra o escalonador ULT, a fila EDF e as threads de Recepção, Atendimento e Teclado.
- **`analista`**: Binário do processo assíncrono de análise de PIDs.
- **`cliente`**: Binário da aplicação cliente fornecida.

#### Arquivos Objeto (`.o`)
- `fila.o`: Fila de prioridade EDF.
- `escalonador.o`: Troca de contexto manual `<ucontext.h>`.
- `atendente.o`: Lógica de atendimento e cálculo de satisfação.
- `main.o`: Ponto de entrada e gerenciamento de ciclo de vida.
- `recepcao.o`: Criação de clientes e controle de fluxo da fila.
- `teclado.o`: Leitura da tecla `'s'` para encerramento.

### Limpeza de Artefatos

Para remover todos os arquivos executáveis, objetos `.o` e arquivos temporários de execução (`lng.txt`, `demanda.txt`):

```bash
make clean
```

---

## 🚀 Execução

O executável `./atendimento` requer dois argumentos obrigatórios via linha de comando:

```bash
./atendimento <N_clientes> <X_paciencia_ms>
```

- **`N_clientes`**: Quantidade total de clientes a serem gerados ($0$ para modo contínuo/infinito).
- **`X_paciencia_ms`**: Tempo máximo de paciência (em milissegundos) para clientes de prioridade Baixa (clientes de prioridade Alta possuem paciência $X/2$ ms).

---

## 📋 Exemplos de Uso

### 1. Execução com Quantidade Determinada de Clientes ($N > 0$)

Simulação com 20 clientes e paciência $X = 500\text{ ms}$:

```bash
./atendimento 20 500
```

### 2. Execução no Modo Infinito ($N = 0$)

Simulação com geração contínua de clientes e paciência $X = 500\text{ ms}$:

```bash
./atendimento 0 500
```

- **Interrupção:** O programa continuará gerando clientes até que a tecla **`s`** ou **`S`** seja pressionada no terminal.
- **Esvaziamento Garantido:** Após a leitura da tecla `'s'`, a thread Recepção para de gerar novos clientes, o Atendente esvazia a fila existente e o sistema dispara ciclos de sinalização ao processo Analista até que o arquivo `lng.txt` fique 100% limpo antes do encerramento final.