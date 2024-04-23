#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define QUEUE_SIZE 10
#define N_ATUADORES 5

// filinha de merda
int queue[QUEUE_SIZE];
int front = 0, rear = 0;
int count = 0;

// o mutex e variavel de condição para sironizar
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_produtor = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_consumidor = PTHREAD_COND_INITIALIZER;





int N_SENSORES;

typedef struct{
	INT atividade;
} Atuador;


Atuador atuadores[N_ATUADORES];
// func produtora/sensor
void *produtor(void *arg) {
	int id = *((int *)arg);
    while (1) {
        int dado = rand() % 1001; //rand

        pthread_mutex_lock(&mutex);

        // Aespera que tenha espaço na fial
        while (count == QUEUE_SIZE) {
            pthread_cond_wait(&cond_produtor, &mutex);
        }

        // add  dado na fila
        queue[rear] = dado;
        rear = (rear + 1) % QUEUE_SIZE;
        count++;

        printf("Produtor: %d: gerou dado %d\n",id, dado);

        // sinaliza para o consumidor que existem dados on's
        pthread_cond_signal(&cond_consumidor);

        pthread_mutex_unlock(&mutex);

        // 1 e 5 segundos
        sleep(rand() % 5 + 1);
    }
    return NULL;
}

// func consumidora
void *consumidor(void *arg) {
    while (1) {
        // bloq o acesso fila
        pthread_mutex_lock(&mutex);

        // espera até que tenha dados  na fila
        while (count == 0) {

            pthread_cond_wait(&cond_consumidor, &mutex);
        }

        // remove um dado da fila
        int dado = queue[front];
        front = (front + 1) % QUEUE_SIZE;
        count--;

        printf("Consumidor: processou dado %d\n", dado);

        // sinaliza tens espaço na fila
        pthread_cond_signal(&cond_produtor);

        pthread_mutex_unlock(&mutex);

        sleep(rand() % 5 + 1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
	//verefica se o número de sensores foi fornecido
	if (argc != 2) {
		printf("Uso %s <N_SENSORES>\n", argv[0]);
		return 1;
	}
    srand(time(NULL)); // Inicializa o gerador de números aleatórios

	// obtem nnumero de sensores
	N_SENSORES = atoi(argv[1]);

	//cria arrays pra armazenar as threads produtoras
	pthread_t thread_produtor[N_SENSORES];
	//cria threads produtoras
	for (int i = 0; i < N_SENSORES; i++){
		int *id = malloc(sizeof(int));
		*id = i;// atribui um id para cada thread produtora
	    pthread_create(&thread_produtor[i], NULL, produtor, id);

	}

	pthread_t thread_consumidor;// cria thread consumidora
    pthread_create(&thread_consumidor, NULL, consumidor, NULL);

	//espera threads produtoras terminarem. obs: não tão terminando KSKS
	for (int i = 0; i < N_SENSORES; i++){
		pthread_join(thread_produtor[i], NULL);
	}
    // aguarda as threads terminarem, mas não tão terminando
    pthread_join(thread_consumidor, NULL);

    return 0;
}
