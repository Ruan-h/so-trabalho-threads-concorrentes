# Trabalho 1 - Sistemas Operacionais (UFC)

Implementação em C (Linux/POSIX) de um simulador de atendimento concorrente utilizando troca de contexto em nível de usuário (`<ucontext.h>`), gerenciamento de processos via `vfork()`/`execl()`, sincronização por semáforos POSIX nomeados e tratamento assíncrono de sinais.

---

## Estrutura dos Módulos

- **`estruturas.h`**: Contrato de interfaces do sistema contendo a definição de `struct Cliente`, protótipos da fila e do escalonador, e variáveis globais compartilhadas (`extern volatile`).
- **`main.c`**: Inicialização dos semáforos POSIX (`/sem_atend` e `/sem_block`), criação e sincronização de partida do processo `analista`, registro das threads no escalonador, drenagem final de `lng.txt` e cálculo das métricas de desempenho.
- **`escalonador.c`**: Escalonador cooperativo *Round-Robin* de threads em nível de usuário (*User-Level Threads*) implementado com `<ucontext.h>` (`getcontext`, `makecontext`, `swapcontext`) e pilhas dedicadas de 64 KB por contexto, sem uso de `pthread`.
- **`fila.c`**: Fila de prioridade com capacidade máxima de 100 clientes em espera, ordenada por **EDF (*Earliest Deadline First*)** com desempate pelo menor tempo de atendimento.
- **`recepcao.c`**: Thread responsável pela criação de processos `cliente` (`vfork()` + `execl()`), sincronização de leitura de `demanda.txt` via `waitpid(..., WUNTRACED)` e `pread()`, sorteio de prioridades (50% Alta / 50% Baixa) e inserção na fila.
- **`atendente.c`**: Thread responsável por retirar o próximo cliente da fila EDF, retomar sua execução com `SIGCONT`, aguardar a conclusão do atendimento sincronizado por `/sem_atend`, contabilizar a satisfação, registrar os PIDs em `lng.txt` sob proteção de `/sem_block` e acionar o `analista` via `SIGUSR1`.
- **`analista.c`**: Processo independente que registra seu PID em `analista_pid.txt` e aguarda sinais `SIGUSR1` com `sigprocmask()` e `sigsuspend()`. Ao ser acordado, lê e remove até os 10 primeiros PIDs de `lng.txt` sob exclusão mútua de `/sem_block`.
- **`teclado.c`**: Thread ativa no modo contínuo ($N = 0$) que configura o terminal em modo não-canônico (`termios`, `~ICANON | ~ECHO`) e monitora `STDIN_FILENO` de forma não-bloqueante via `poll()` para capturar a tecla `'s'` ou `'S'`.
- **`cliente.c`**: Código-fonte do processo cliente conforme especificação do trabalho.
- **`Makefile`**: Regras de compilação dos binários `atendimento`, `analista` e `cliente`.

---

## Arquitetura e Mecanismos de Sincronização

| Mecanismo / Recurso | Processos / Threads Envolvidos | Função no Sistema |
|---|---|---|
| `<ucontext.h>` (`yield_thread`) | `recepcao`, `atendente`, `teclado` | Troca de contexto cooperativa em espaço de usuário dentro do processo `atendimento`. |
| `demanda.txt` + `SIGSTOP` (`WUNTRACED`) | `cliente` → `recepcao` | O cliente grava seu tempo de serviço, fecha o arquivo e executa `raise(SIGSTOP)`. A `recepcao` aguarda `WIFSTOPPED` antes de ler a demanda. |
| `SIGCONT` + `/sem_atend` + `waitpid` | `atendente` ↔ `cliente` | O `atendente` acorda o cliente com `SIGCONT`; o cliente executa o atendimento protegido por `/sem_atend` e encerra, sendo recolhido por `waitpid`. |
| `lng.txt` + `/sem_block` | `atendente` ↔ `analista` | Semáforo POSIX nomeado que garante exclusão mútua entre a escrita de PIDs atendidos e a leitura/truncamento pelo `analista`. |
| `SIGUSR1` + `sigsuspend()` | `atendente` / `main` → `analista` | Acorda o processo `analista` para imprimir até 10 PIDs na saída padrão e removê-los de `lng.txt` via `ftruncate()`. |
| `analista_pid.txt` + `SIGTERM` | `main` ↔ `analista` | O `analista` grava seu PID após configurar a máscara de sinais; no encerramento, após `lng.txt` atingir 0 bytes, o `main` finaliza o `analista` com `SIGTERM`. |

### Política de Escalonamento da Fila e Cálculo de Satisfação
- **Paciência por Prioridade:** Clientes de prioridade Baixa possuem paciência $X\text{ ms}$ e clientes de prioridade Alta possuem paciência $X/2\text{ ms}$ (50% de probabilidade para cada tipo).
- **Ordenação EDF + SJF:** A fila prioriza o cliente com menor *deadline* absoluto ($\text{hora\_chegada} + \text{tempo\_paciencia}$). Em caso de empate de *deadline*, prioriza o menor `tempo_atendimento` (`1`, `5` ou `15`).
- **Controle de Fluxo e Encerramento ($N = 0$):** Quando a fila atinge 100 clientes, a `recepcao` cede a CPU até abrir vaga. Ao pressionar `'s'` no modo $N = 0$, a geração e o atendimento param imediatamente, os processos não atendidos restantes na fila são finalizados (`SIGKILL` + `waitpid`) e ignorados na taxa de satisfação, e o `analista` é acionado até esvaziar `lng.txt`.

---

## Compilação

Para compilar todos os executáveis (`atendimento`, `analista` e `cliente`):

```bash
make
```

Para remover binários, objetos `.o` e arquivos temporários de execução:

```bash
make clean
```

---

## Execução

```bash
./atendimento <N_clientes> <X_paciencia_ms>
```

- **`N_clientes`**: Total de clientes a serem gerados ($0$ para geração contínua até pressionar `'s'`).
- **`X_paciencia_ms`**: Tempo máximo de paciência em milissegundos para clientes de prioridade Baixa ($X/2\text{ ms}$ para prioridade Alta).

### Exemplos de Uso

1. **Quantidade fixa de clientes ($N > 0$):**
   ```bash
   ./atendimento 20 500
   ```

2. **Modo contínuo ($N = 0$):**
   ```bash
   ./atendimento 0 500
   ```
   Pressione `s` ou `S` no terminal para encerrar o atendimento, drenar `lng.txt` e exibir as métricas finais.
