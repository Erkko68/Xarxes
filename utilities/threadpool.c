#include "commons.h"

/* Define a struct to represent each thread */
typedef struct task {
  void (*function)(void*); /* Function pointer of the thread */
  void* arg;               /* Argument to be passed to the function */
} task_t;

/*  Define a thread pool struct */
typedef struct threadpool {
  int num_threads;          /* Number of threads in the pool */
  pthread_t* threads;       /* Array of thread IDs */
  task_t* queue;            /* Queue for tasks */
  int queue_size;           /* Size of the queue */
  int head, tail;           /* Head and tail indices for the queue */
  pthread_mutex_t mutex;    /* Mutex for thread safety */
  pthread_cond_t not_empty; /* Condition variable to signal non-empty queue */
  pthread_cond_t not_full;  /* Condition variable to signal non-full queue */
} threadpool_t;

/* Function to initialize the thread pool */
threadpool_t* threadpool_init(int num_threads, int queue_size) {
  threadpool_t* pool = malloc(sizeof(threadpool_t));
  if (!pool) {
    return NULL;
  }

  pool->num_threads = num_threads;
  pool->queue_size = queue_size;
  pool->queue = malloc(sizeof(task_t) * queue_size);
  pool->threads = malloc(sizeof(pthread_t) * num_threads);

  pool->head = pool->tail = 0;
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->not_empty, NULL);
  pthread_cond_init(&pool->not_full, NULL);

  /* Create worker threads */
  for (int i = 0; i < num_threads; i++) {
    if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
      threadpool_destroy(pool);
      return NULL;
    }
  }

  return pool;
}

/* Worker thread function */
void* worker_thread(void* arg) {
  threadpool_t* pool = (threadpool_t*)arg;
  while (true) {
    pthread_mutex_lock(&pool->mutex);

    /* Wait for a non-empty queue */
    while (pool->head == pool->tail) {
      pthread_cond_wait(&pool->not_empty, &pool->mutex);
    }

    /* Get the task from the queue */
    task_t task = pool->queue[pool->head];
    pool->head = (pool->head + 1) % pool->queue_size;

    pthread_mutex_unlock(&pool->mutex);

    /* Execute the task */
    (*task.function)(task.arg);
  }
  return NULL;
}

/* Function to add a task to the pool */
int threadpool_add_work(threadpool_t* pool, void (*function)(void*), void* arg) {
  pthread_mutex_lock(&pool->mutex);

  /* Wait for space in the queue */
  while ((pool->tail + 1) % pool->queue_size == pool->head) {
    pthread_cond_wait(&pool->not_full, &pool->mutex);
  }

  /* Add the task to the queue */
  pool->queue[pool->tail].function = function;
  pool->queue[pool->tail].arg = arg;
  pool->tail = (pool->tail + 1) % pool->queue_size;

  /* Signal a non-empty queue */
  pthread_cond_signal(&pool->not_empty);
  pthread_mutex_unlock(&pool->mutex);

  return 0;
}

/* Function to destroy the thread pool */
void threadpool_destroy(threadpool_t* pool) {
  if (pool == NULL) {
    return;
  }

  pthread_cond_destroy(&pool->not_empty);
  pthread_cond_destroy(&pool->not_full);
  pthread_mutex_destroy(&pool->mutex);

  if (pool->threads) {
    for (int i = 0; i < pool->num_threads; i++) {
      pthread_join(pool->threads[i], NULL);
    }
    free(pool->threads);
  }

  if (pool->queue) {
    free(pool->queue);
  }
  free(pool);
}
