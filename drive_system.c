#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define QUEUE_SIZE 16
#define N_UN_PROCESSAMENTO 4

#define PRINT

int N_SENSORES, N_ATUADORES;
int queue[QUEUE_SIZE];
int head = 0;
int tail = 0;
int *package[3];

void *produtor(void *arg)
{
  int id = *(int *) arg;

  do
  {
    int dado_sensorial = rand() % 1000;
    queue[tail] = dado_sensorial;
    tail = (tail + 1) % QUEUE_SIZE;

    #ifdef PRINT
    printf("Sensor %d leu %d\n", id, dado_sensorial);
    #endif

    sleep(rand() % 5);
  } while (1);

  return NULL;
}

void *consumidor(void *arg)
{
  int *package = (int *) arg;
  int id = package[0];
  int nivel_atividade = package[1];
  int atuador = package[3];

  int pid = fork();

  // envio da mudança do nível do painel
  if (pid != 0)
  {
    int falha;
    printf("Alterando: %s com valor %s\n", id, nivel_atividade);
    sleep(1);
  }
  // mudança no nível de atividade
  else
  {
    int falha;
    atuador = nivel_atividade;
    sleep(2 + rand() % 2);
  }

  return NULL;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Para executar o programa deve escrever '%s <numero_sensores> <numero_atuadores>'\n", argv[0]);
    return 1;
  }

  // inicializa gerador de números aleatórios
  srand(time(NULL));

  // obtem numero de sensores
  N_SENSORES = atoi(argv[1]);
  N_ATUADORES = atoi(argv[2]);

  int i, atuador[N_ATUADORES], dado_sensorial;

  // cria array para armazenar as threads produtoras
  pthread_t t_produtor[N_SENSORES];
  pthread_t t_un_processamento[N_UN_PROCESSAMENTO];

  // cria threads produtoras
  for (i = 0; i < N_SENSORES; i++)
    pthread_create(&t_produtor[i], NULL, produtor, (void *) &i);

  // cria pool de threads
  // printf("Dormindo...\n");
  // sleep(10);
  
  while (tail != head)
  {
    #ifdef PRINT
    printf("Central de controle recebeu %i\n", queue[head]);
    #endif
    
    dado_sensorial = queue[head];
    head = (head + 1) % QUEUE_SIZE;
  }
  // atribui os dados sensoriais a uma unidade de processamento
  int id_atuador = dado_sensorial % N_ATUADORES;
  int nivel_atividade = rand() % 100;
  package[0] = &id_atuador;
  package[1] = &nivel_atividade;
  package[2] = &atuador[id_atuador];
  pthread_create(&t_un_processamento[id_atuador % N_UN_PROCESSAMENTO], NULL, consumidor, (void *) &package);

  // junta threads produtoras
  for (i = 0; i < N_SENSORES; i++)
    pthread_join(t_produtor[i], NULL);

  return 0;
}
