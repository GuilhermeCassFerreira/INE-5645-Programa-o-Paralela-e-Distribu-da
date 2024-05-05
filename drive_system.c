#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define QUEUE_SIZE 16
#define N_UN_PROCESSAMENTO 4

struct actuator
{
  int nivel_atividade;
};

struct data
{
  int id;
  int nivel_atividade;
};

int N_SENSORES, N_ATUADORES;
int queue[QUEUE_SIZE];
int head = 0;
int tail = 0;
struct actuator atuadores[2];

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_full_cond = PTHREAD_COND_INITIALIZER;

void *producer(void *args)
{
  int dado_sensorial, i;

  for (i = 0; i < 15; i++)
  {
    dado_sensorial = rand() % 1000;

    pthread_mutex_lock(&queue_mutex);
    while ((tail + 1) % QUEUE_SIZE == head)
      pthread_cond_wait(&queue_full_cond, &queue_mutex);

    queue[tail] = dado_sensorial;
    tail = (tail + 1) % QUEUE_SIZE;

    printf("Sensor leu %d\n", dado_sensorial);

    pthread_cond_signal(&queue_empty_cond);
    pthread_mutex_unlock(&queue_mutex);

    sleep(rand() % 5);
  }

  pthread_exit(NULL);
}

void *consumer(void *args)
{
  struct data new_data;

  for (int i = 0; i < 16; i++)
  {
    pthread_mutex_lock(&queue_mutex);
    while (head == tail)
      pthread_cond_wait(&queue_empty_cond, &queue_mutex);

    int dado_sensorial = queue[head];
    head = (head + 1) % QUEUE_SIZE;

    printf("Central de controle recebeu %i\n", dado_sensorial);

    new_data.id = dado_sensorial % N_ATUADORES;
    new_data.nivel_atividade = rand() % 100;

    pthread_mutex_unlock(&queue_mutex);

    struct actuator *atuador = &atuadores[new_data.id - 1];

    if (fork() == 0)
    {
      printf("Alterando: %i com valor %i\n", new_data.id, new_data.nivel_atividade);
      sleep(1);
      exit(1);
    }
    else
    {
      pthread_mutex_lock(&queue_mutex);
      atuador->nivel_atividade = new_data.nivel_atividade;
      sleep(2 + rand() % 2);
      pthread_mutex_unlock(&queue_mutex);
    }
  }

  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Para executar o programa deve escrever '%s <numero_sensores> <numero_atuadores>'\n", argv[0]);
    return 1;
  }

  srand(time(NULL));

  N_SENSORES = atoi(argv[1]);
  N_ATUADORES = atoi(argv[2]);

  pthread_t producer_thread, consumer_thread;
  
  pthread_create(&producer_thread, NULL, producer, NULL);
  pthread_create(&consumer_thread, NULL, consumer, NULL);

  pthread_join(producer_thread, NULL);
  pthread_join(consumer_thread, NULL);

  return 0;
}
