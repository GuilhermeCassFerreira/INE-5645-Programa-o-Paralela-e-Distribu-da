#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

typedef struct task
{
  void (*function)(void *);
  void *arg;
  struct task *next;
} task_t;

typedef struct thread_pool
{
  int num_threads;
  pthread_t *threads;
  task_t *task_queue;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  int shutdown;
} thread_pool_t;

void thread_pool_init(thread_pool_t *pool, int num_threads);
void thread_pool_submit(thread_pool_t *pool, void (*function)(void *), void *arg);
void thread_pool_shutdown(thread_pool_t *pool);

#endif