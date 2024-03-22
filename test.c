#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_THREADS 10
#define MAX_QUEUE_SIZE 1000

typedef struct {
    void (*function)(void*);
    void *argument;
} task_t;

typedef struct {
    task_t tasks[MAX_QUEUE_SIZE];
    int head, tail, count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty, not_full;
    int shutdown;
    pthread_t threads[MAX_THREADS];
} thread_pool_t;

void* worker(void *arg) {
    thread_pool_t *pool = (thread_pool_t*)arg;
    while (1) {
        pthread_mutex_lock(&pool->lock);
        while (pool->count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }
        task_t task = pool->tasks[pool->head];
        pool->head = (pool->head + 1) % MAX_QUEUE_SIZE;
        pool->count--;
        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->lock);

        (task.function)(task.argument);
        free(task.argument); // Free memory after task execution
    }
}

thread_pool_t* thread_pool_create() {
    thread_pool_t *pool = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    pool->head = pool->tail = pool->count = 0;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);
    pool->shutdown = 0;

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&pool->threads[i], NULL, worker, (void*)pool);
    }
    return pool;
}

void thread_pool_submit(thread_pool_t *pool, void (*function)(void*), void *argument) {
    pthread_mutex_lock(&pool->lock);
    while (pool->count == MAX_QUEUE_SIZE) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }
    pool->tasks[pool->tail].function = function;
    pool->tasks[pool->tail].argument = argument;
    pool->tail = (pool->tail + 1) % MAX_QUEUE_SIZE;
    pool->count++;
    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
}

void thread_pool_shutdown(thread_pool_t *pool) {
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->lock);
    pthread_cond_broadcast(&pool->not_empty);
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
    free(pool);
}

void example_task(void *arg) {
    long *num = (long*)arg;
    printf("Task executed with argument: %ld\n", *num);
}

int main() {
    thread_pool_t *pool = thread_pool_create();

    for (int i = 0; i < 20; i++) {
        long *num = (long*)malloc(sizeof(long));
        *num = i;
        thread_pool_submit(pool, example_task, (void*)num);
    }

    // Wait a bit to ensure all tasks are executed
    sleep(5);

    thread_pool_shutdown(pool);

    return 0;
}
