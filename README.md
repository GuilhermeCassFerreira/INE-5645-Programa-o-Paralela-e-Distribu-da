# Sistema de acionamento automático em carros autônomos

## Funcionamento

O sistema foi desenvolvido na linguagem de programação C e utiliza _Posix Threads_ para a criação de threads paralelas, além de implementar a _pool de threads_.

Inicialmente, foi criado um documento para sintetizar os requisitos pedidos no trabalho, os quais serviram como base para criação de issues e sua divisão entre os integrantes do grupo. Após isso, o foco da equipe foi atender o funcionamento base de forma sequencial.

Uma vez com o código sequencial funcional, o foco da equipe passou a ser a sua paralelização. A divisão do trabalho nesta parte se deu de forma em que um integrante ficou responsável pela sincronização das threads (a fim de evitar condições de corrida e demais problemas provenientes da execução em paralelo, para isso foi utilizado exclusão mútua e variáveis de condição) e outro ficou responsável pela implementação da pool de threads, a qual é explicada em uma seção abaixo.


### Paralelização do Código Sequencial

A paralelização do código sequencial foi alcançada por meio da utilização de múltiplas threads para processar tarefas de forma simultânea. Para garantir a integridade dos dados e a sincronização entre as threads, foram empregados mecanismos de exclusão mútua e variáveis de condição.

### Estrutura de Sincronização das Threads

Foi criada uma estrutura pthread_mutex_t (actuator_mutex) para garantir a exclusão mútua no acesso e modificação dos atuadores compartilhados entre as threads. Isso evita condições de corrida, garantindo que apenas uma thread por vez possa alterar o estado dos atuadores.

Além disso, uma estrutura pthread_mutex_t adicional (print_mutex) foi utilizada para controlar o acesso à saída padrão (stdout) durante a impressão das mensagens no console. Isso evita que múltiplas threads escrevam simultaneamente na saída padrão, o que poderia resultar em mensagens sobrepostas ou mal formatadas.

### Implementação da Fila de Tarefas

Para processar as tarefas de forma paralela, foi adotada uma abordagem de produtor-consumidor, onde os produtores são as threads que geram os dados sensoriais (sensores) e o consumidor é uma threads que processa esses dados (central de controle) e delega à pool de threads (unidades de processamento) que controla os atuadores.

Foi implementada uma fila de tarefas utilizando um array queue, onde os dados sensoriais são armazenados para posterior processamento pela thread consumidora. Essa fila é protegida por exclusão mútua, garantindo que apenas uma thread por vez possa adicionar ou remover elementos da fila.

### Coordenação das Threads

A coordenação entre as threads é realizada por meio de variáveis de condição (queue_empty_cond e queue_full_cond). Quando a fila de tarefas está vazia, as threads consumidoras aguardam em uma variável de condição (queue_empty_cond) até que novas tarefas sejam adicionadas à fila. Da mesma forma, quando a fila de tarefas está cheia, as threads produtoras aguardam em uma variável de condição (queue_full_cond) até que haja espaço disponível para adicionar novas tarefas.

### Execução Paralela das Tarefas

As tarefas são processadas de forma paralela pelas threads da pool de threads, que executam a função `actuate`. Cada thread da pool (unidade de processamento) retira uma tarefa da fila de tarefas e a processa, garantindo que múltiplas tarefas possam ser executadas simultaneamente.

### Encerramento do Sistema

Ao final da execução do programa, é realizado o encerramento adequado das threads e a liberação dos recursos alocados na memória. Isso é feito de forma coordenada, garantindo que todas as threads sejam finalizadas corretamente e que não ocorram vazamentos de memória.

Essas estratégias de sincronização e coordenação entre as threads permitem que o sistema funcione de forma eficiente e confiável em um ambiente de execução paralela, garantindo a integridade dos dados e o correto fluxo de execução entre as threads.

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

##Exemplo de execução

![Logo da Minha Empresa](https://drive.google.com/file/d/1maxjTN9a9abwVclkPu4P9zpYpXwCjnHB/view?usp=sharing)

Podemos ver nesse exemplo onde executamos o programa com 6 sensores e 6 atuadores, em que habilitamos o log, a pool de threads sendo criada após a
validação das entradas do usuário que é usada para alocar memória para o vetor de tamanho dinâmico.
Logo após a criação da pool de threads as threads produtoras são criadas, no caso dessa execução 6 threads produtoras são criadas. Em seguida a 
thread consumidora é criada.
No terminal vemos que após a sinalização da criação do pool de threads já vemos dados sensoriais sendo criados, já que antes da execução chegar na 
próxima linha onde está o print as threads já estão trabalhando. Vemos que o log mostra a criaçaõ das threads produtoras e antes de aparecer a criação
da thread consumidora mais dados sensorias são criados, onde é usado o mutex para que cada thread produtora não acesse de maneira simultanea a fila e
consiga colocar o dado sensorial na proxima posição da fila para processamento.
Podemos observar a execução da thread consumidora consumindo os dados da fila nas saídas identificadas por [C]. Também vemos as saídas que mostram o os dados com seu id e seu nível de ativide, mandado como parâmetro para o pool de threads.

## Estudantes

- Bruno Vazquez Lafaiete
- Cauã Pablo Padilha
- Guilherme Cassiano Ferreira

## Docentes

- Odorico Machado Mendizabal
- Douglas Pereira Luiz
