#include "threadpool.h"
#include <stdlib.h>

void *thread_pool_worker(void *arg)
{
  thread_pool_t *pool = (thread_pool_t *) arg;

  while(1)
  {
    pthread_mutex_lock(&pool->lock);
    while (pool->task_queue == NULL && !pool->shutdown)
      pthread_cond_wait(&pool->cond, &pool->lock);
    
    if (pool->shutdown)
    {
      pthread_mutex_unlock(&pool->lock);
      pthread_exit(NULL);
    }

    task_t *task = pool->task_queue;
    pool->task_queue = task->next;
    pthread_mutex_unlock(&pool->lock);
    task->function(task->arg);
    free(task);
  }
}

void thread_pool_init(thread_pool_t *pool, int num_threads)
{
  pool->num_threads = num_threads;
  pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);
  pool->task_queue = NULL;
  pthread_mutex_init(&pool->lock, NULL);
  pthread_cond_init(&pool->cond,  NULL);
  pool->shutdown = 0;

  for (int i = 0; i < num_threads; ++i)
    pthread_create(&pool->threads[i], NULL, thread_pool_worker, (void *) pool);
}

void thread_pool_submit(thread_pool_t *pool, void (*function)(void *), void *arg)
{
  task_t *task = (task_t *) malloc(sizeof(task_t));
  task->function = function;
  task->arg = arg;
  task->next = NULL;

  pthread_mutex_lock(&pool->lock);

  if (pool->task_queue == NULL)
    pool->task_queue = task;
  else
  {
    task_t *tmp = pool->task_queue;

    while (tmp->next != NULL)
      tmp = tmp->next;
    
    tmp->next = task;
  }

  pthread_cond_signal(&pool->cond);
  pthread_mutex_unlock(&pool->lock);
}

void thread_pool_shutdown(thread_pool_t *pool)
{
  pthread_mutex_lock(&pool->lock);
  pool->shutdown = 1;
  pthread_cond_broadcast(&pool->cond);
  pthread_mutex_unlock(&pool->lock);

  for (int i = 0; i < pool->num_threads; ++i)
    pthread_join(pool->threads[i], NULL);
  
  free(pool->threads);
  task_t *task = pool->task_queue;

  while (task != NULL)
  {
    task_t *next_task = task->next;
    free(task);
    task = next_task;
  }

  pthread_mutex_destroy(&pool->lock);
  pthread_cond_destroy(&pool->cond);
}
