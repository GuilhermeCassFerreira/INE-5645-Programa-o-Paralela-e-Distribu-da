#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h> //depedencia para poder esperar o processo filho
#include "threadpool.h"

#define QUEUE_SIZE 16
#define N_UN_PROCESSAMENTO 4

// #define LOG

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
struct actuator *atuadores;

pthread_mutex_t actuator_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_full_cond = PTHREAD_COND_INITIALIZER;

void actuate(void *arg) {
  struct data *new_data_ptr = (struct data *) arg;
  struct data new_data = *new_data_ptr;

  struct actuator *atuador = &atuadores[new_data.id];
  int falha_envio_painel = 0;
  int falha_atuador = 0;

  pid_t pid_envio_painel = fork();

  if (pid_envio_painel == 0) {
    printf("Processo filho para o envio de mudanças ao painel criado (PID: %d)\n", getpid());

    if (rand() % 100 < 20) {
      falha_envio_painel = 1; 
    } else {
      pthread_mutex_lock(&print_mutex);
      printf("Alterando: %i com valor %i\n", new_data.id, new_data.nivel_atividade);
      pthread_mutex_unlock(&print_mutex);
      sleep(1);
    }
    exit(falha_envio_painel ? 1 : 0); 
  } else if (pid_envio_painel < 0) {
    perror("Erro ao criar processo filho para o envio de mudanças ao painel");
    exit(EXIT_FAILURE);
  }

  pid_t pid_atuador = fork();

  if (pid_atuador == 0) {
    printf("Processo filho para a mudança no nível de atividade do atuador criado (PID: %d)\n", getpid());

    if (rand() % 100 < 20) {
      falha_atuador = 1; //
    } else {
      pthread_mutex_lock(&actuator_mutex);
      atuador->nivel_atividade = new_data.nivel_atividade;
      pthread_mutex_unlock(&actuator_mutex);
      sleep(2 + rand() % 2);
    }
    exit(falha_atuador ? 1 : 0); 
  } else if (pid_atuador < 0) {
    perror("Erro ao criar processo filho para a mudança no nível de atividade do atuador");
    exit(EXIT_FAILURE);
  }

  printf("Processo pai esperando pelos processos filhos...\n");
  int status_envio_painel, status_atuador;
  waitpid(pid_envio_painel, &status_envio_painel, 0);
  waitpid(pid_atuador, &status_atuador, 0);

  printf("Processo pai concluiu a espera pelos processos filhos.\n");

  if (WIFEXITED(status_envio_painel) && WEXITSTATUS(status_envio_painel) == 1) {
    falha_envio_painel = 1;
  }

  if (WIFEXITED(status_atuador) && WEXITSTATUS(status_atuador) == 1) {
    falha_atuador = 1;
  }

  if (falha_envio_painel || falha_atuador) {
    printf("Falha: %i\n", new_data.id);
  }
}





void *producer()
{
  while (1)
  {
    int dado_sensorial = rand() % 1000;

    pthread_mutex_lock(&queue_mutex);
    while ((tail + 1) % QUEUE_SIZE == head)
      pthread_cond_wait(&queue_full_cond, &queue_mutex);

    queue[tail] = dado_sensorial;
    tail = (tail + 1) % QUEUE_SIZE;

    #ifdef LOG
    printf("[P] dado_sensorial: %i | tail: %i | novo tail: %i\n", dado_sensorial, (tail - 1) % QUEUE_SIZE, tail);
    #endif

    pthread_cond_signal(&queue_empty_cond);
    pthread_mutex_unlock(&queue_mutex);

    sleep(rand() % 5);
  }

  pthread_exit(NULL);
}

void *consumer(void *arg)
{
  while (1)
  {
    pthread_mutex_lock(&queue_mutex);
    while (head == tail)
      pthread_cond_wait(&queue_empty_cond, &queue_mutex);

    struct data new_data;
    thread_pool_t *pool = (thread_pool_t *) arg;

    int dado_sensorial = queue[head];
    head = (head + 1) % QUEUE_SIZE;

    #ifdef LOG
    printf("[C] dado_sensorial: %i | head: %i | novo head: %i\n", dado_sensorial, (head - 1) % QUEUE_SIZE, head);
    #endif

    new_data.id = dado_sensorial % N_ATUADORES;
    new_data.nivel_atividade = rand() % 100;

    #ifdef LOG
    printf("[C] nivel_atividade: %i | id: %i\n", new_data.nivel_atividade, new_data.id);
    #endif

    thread_pool_submit(pool, actuate, &new_data);
    pthread_cond_signal(&queue_full_cond);
    pthread_mutex_unlock(&queue_mutex);
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
  atuadores = calloc(N_ATUADORES, sizeof(struct actuator));

  pthread_t producer_thread[N_SENSORES], consumer_thread;
  thread_pool_t pool;

  #ifdef LOG
  printf("[M] Pool de threads criado\n");
  #endif

  thread_pool_init(&pool, N_UN_PROCESSAMENTO);

  for (int i = 0; i < N_SENSORES; i++)
    pthread_create(&producer_thread[i], NULL, producer, NULL);

  #ifdef LOG
  printf("[M] Todas threads produtoras criadas\n");
  #endif

  pthread_create(&consumer_thread, NULL, consumer, &pool);

  #ifdef LOG
  printf("[M] Thread consumidora criada\n");
  #endif

  for (int i = 0; i < N_SENSORES; i++)
    pthread_join(producer_thread[i], NULL);

  #ifdef LOG
  printf("[M] Todas threads produtoras finalizadas\n");
  #endif

  pthread_join(consumer_thread, NULL);

  #ifdef LOG
  printf("[M] Thread consumidora finalizada\n");
  #endif

  thread_pool_shutdown(&pool);

  #ifdef LOG
  printf("[M] Pool de threads finalizado\n");
  #endif

  free(atuadores);
  pthread_mutex_destroy(&actuator_mutex);
  pthread_mutex_destroy(&print_mutex);
  pthread_mutex_destroy(&queue_mutex);
  pthread_cond_destroy(&queue_empty_cond);
  pthread_cond_destroy(&queue_full_cond);

  return 0;
}
