# Sistema de acionamento automático em carros autônomos

## Funcionamento

O sistema foi desenvolvido na linguagem de programação C e utiliza _Posix Threads_ para a criação de threads paralelas, além de implementar a _pool de threads_.

Inicialmente, foi criado um documento para sintetizar os requisitos pedidos no trabalho, os quais serviram como base para criação de issues e sua divisão entre os integrantes do grupo. Após isso, o foco da equipe foi atender o funcionamento base de forma sequencial.

Uma vez com o código sequencial funcional, o foco da equipe passou a ser a sua paralelização. A divisão do trabalho nesta parte se deu de forma em que um integrante ficou responsável pela sincronização das threads (a fim de evitar condições de corrida e demais problemas provenientes da execução em paralelo, para isso foi utilizado exclusão mútua e variáveis de condição) e outro ficou responsável pela implementação da pool de threads, a qual é explicada em uma seção abaixo.

### Pool de threads

A pool de threads foi implementada utilizando de exclusão mútua e variáveis de condição e funciona da seguinte forma:

É criada uma estrutura da pool de threads (`thread_pool_t`), que possui as seguintes variáveis:
  - número de threads
  - ponteiro para threads
  - ponteiro para uma fila de tasks (será explicado adiante)
  - mutex
  - variável de condição
  - indicador do estado da pool (ligada/desligada)

É iniciada a pool de threads (`thread_pool_init`), em que é enviada a estrutura criada e o número de threads desejadas (esse valor é constante no código e é definido por `N_UN_PROCESSAMENTO`).
    
A inicialização da pool de threads primeiramente define o número de threads com o valor recebido e aloca espaço na memória com o tamanho de uma thread x número de threads. Após isso é definida a fila de tasks vazia, inicializa-se o mutex e a variável de condição, por fim define o estado da pool como ligado (`0`). Com isso, são iniciadas as threads com `pthread_create` e começam a executar `thread_pool_worker`, onde ficam aguardando com `wait` da variável de condição até serem ativadas para executar alguma task.

Para delegar para a pool de threads executar alguma função é utilizada `thread_pool_submit`, a qual recebe a pool, a função que deseja-se executar e seus argumentos. Após isso, é criada uma task que possui a seguinte estrutura:
  - função a ser executada
  - argumentos para a função
  - ponteiro para a próxima task

Dessa forma, é atribuído a task criada tais respectivos valores (inicialmente com a próxima task nula, mas após isso é verificado na pool a fila de tasks e assim a task criada é apontada pela task anterior, sendo colocada na fila de tasks) e a variável de condição envia `signal` para `thread_pool_worker` que continua sua execução.

Com isso, é verificado se a pool está desligada (se sim, libera o mutex e encerra a thread) e, em caso negativo, pega a primeira task da fila de tasks e executa a função da task com seus respectivos argumentos.

Ao final da execução é encerrada a pool de threads com `thread_pool_shutdown`, o que altera o valor do estado da pool para desligado, libera todas as threads que estão aguardando em `thread_pool_worker` e espera que todas encerrem sua execução. Por fim, libera o espaço alocado pelas threads e task por task na fila de task libera o espaço alocado, depois destruindo o mutex e a variável de condição criadas.

## Compilação e execução

Para compilar o código-fonte basta executar o seguinte comando:

```bash
gcc drive_system.c threadpool.c -o main -pthread
```

Após isso, deve ser executado o comando:

```bash
./main <numero_sensores> <numero_atuadores>
```

> Recomenda-se que o código-fonte seja compilado e executado em uma mesma máquina e em um sistema operacional baseado em Unix.

### Logging

Foi definido uma condição de pré-compilação para o sistema realizar logging do seu funcionamento, as informações são logadas no próprio terminal (o que pode dificultar a leitura pela verbosidade). No início de cada linha de log há um caracter que define em qual parte do sistema foi logado cada informação, abaixo há uma breve legenda do que cada caracter informa:

- [P] Informação logada na função `producer`;
- [C] Informação logada na função `consumer`;
- [A] Informação logada na função `actuate`;
- [M] Informação logada na função `main`.

Para logar no terminal basta descomentar a linha que contém `#define LOG`.

## Estudantes

- Bruno Vazquez Lafaiete
- Cauã Pablo Padilha
- Guilherme Cassiano Ferreira

## Docentes

- Odorico Machado Mendizabal
- Douglas Pereira Luiz
