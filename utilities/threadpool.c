/**
 * @file threadpool.c
 * @brief Methods file for thread pool management.
 * 
 * This c file contains functions implementations related to the management
 * of a thread pool for concurrent task execution.
 * 
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-24
 */

#include "commons.h"

/* Define POISON_PILL task wich tells the workers thread to stop their execution */
#define POISON_PILL NULL

/**
 * @brief Worker function for thread pool.
 *
 * This function represents the worker routine that each thread in the
 * thread pool executes. It continuously waits for tasks to be available
 * in the task queue, executes them, and then waits for the next task.
 * If the shutdown flag is set, the worker thread exits.
 * 
 * @param arg Pointer to the thread pool structure.
 */
void* worker(void *arg) {
    thread_pool_t *pool = (thread_pool_t*)arg;
    while (1) {
        task_t task;
        pthread_mutex_lock(&pool->lock);
        while (pool->count == 0) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }
        task = pool->tasks[pool->head];
        if (task.function == POISON_PILL) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        pool->head = (pool->head + 1) % MAX_QUEUE_SIZE;
        pool->count--;
        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->lock);

        (task.function)(task.argument);
        free(task.argument);
    }
    pthread_exit(NULL);
}

/**
 * @brief Creates a new thread pool.
 *
 * This function dynamically allocates memory for a new thread pool
 * structure, initializes its attributes, and creates worker threads
 * to handle tasks submitted to the pool.
 * 
 * @return Returns a pointer to the newly created thread pool.
 */
thread_pool_t* thread_pool_create() {
    int i;
    thread_pool_t *pool = NULL;
    pool = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    if ( pool == NULL) {
        lerror("Failed to allocate memory for thread pool",true);
    }
    pool->head = pool->tail = pool->count = pool->shutdown = 0;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);

    for (i = 0; i < MAX_THREADS; i++) {
        pthread_create(&pool->threads[i], NULL, worker, (void*)pool);
    }
    return pool;
}

/**
 * @brief Submits a task to the thread pool.
 *
 * This function adds a new task to the task queue of the specified
 * thread pool. If the queue is full, it waits until space becomes
 * available. Once the task is added, it signals the worker threads
 * that a new task is available for execution.
 * 
 * @param pool Pointer to the thread pool.
 * @param function Pointer to the function representing the task.
 * @param argument Pointer to the argument for the task function.
 */
void thread_pool_submit(thread_pool_t *pool, void (*function)(void*), void *argument) {
    pthread_mutex_lock(&pool->lock);
    while (pool->count == MAX_QUEUE_SIZE) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return;
    }
    pool->tasks[pool->tail].function = function;
    pool->tasks[pool->tail].argument = argument;
    pool->tail = (pool->tail + 1) % MAX_QUEUE_SIZE;
    pool->count++;
    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
}


/**
 * @brief Shuts down the thread pool.
 *
 * This function initiates the shutdown process for the thread pool.
 * It sets the shutdown flag, broadcasts to all worker threads that
 * they should exit, waits for all threads to join, and then cleans
 * up the resources associated with the thread pool.
 * 
 * @param pool Pointer to the thread pool to be shut down.
 */
void thread_pool_shutdown(thread_pool_t *pool) {
    int i;

    for (i = 0; i < MAX_THREADS; i++) {
        thread_pool_submit(pool, POISON_PILL, POISON_PILL);
    }

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->lock);

    /* Wait for worker threads to receive POISON_PILL */
    linfo("Closing threads...\n",false);
    sleep((MAX_THREADS/2));

    for (i = 0; i < MAX_THREADS; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_lock(&pool->lock);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
    pthread_mutex_unlock(&pool->lock);

    pthread_mutex_destroy(&pool->lock);
    free(pool);
}