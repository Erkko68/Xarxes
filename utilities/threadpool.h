/**
 * @file threadpool.h
 * @brief Header file for thread pool management.
 * 
 * This header file contains declarations for functions and structures
 * related to the management of a thread pool for concurrent task execution.
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-22
 */

#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include "commons.h"

#define MAX_THREADS 5 /* Maximum number of worker threads in the thread pool. */
#define MAX_QUEUE_SIZE 20 /* Maximum size of the task queue in the thread pool. */

/**
 * @brief Represents a task to be executed by the thread pool.
 */
typedef struct {
    void (*function)(void*); /* Pointer to the function to be executed. */
    void *argument; /* Pointer to the argument for the function. */
} task_t;

/**
 * @brief Represents a thread pool for managing concurrent tasks.
 */
typedef struct {
    task_t tasks[MAX_QUEUE_SIZE]; /* Array to store tasks in the queue. */
    int head, tail, count; /* Indices and count for task queue management. */
    pthread_mutex_t lock; /* Mutex for controlling access to shared data. */
    pthread_cond_t not_empty, not_full; /* Condition variables for synchronization. */
    int shutdown; /* Flag to indicate if the thread pool is being shut down. */
    pthread_t threads[MAX_THREADS]; /* Array to store worker threads. */
} thread_pool_t;

/* Function declarations */

/**
 * @brief Creates a new thread pool.
 * 
 * @return Returns a pointer to the newly created thread pool.
 */
thread_pool_t* thread_pool_create();

/**
 * @brief Submits a task to the thread pool.
 * 
 * @param pool Pointer to the thread pool.
 * @param function Pointer to the function representing the task.
 * @param argument Pointer to the argument for the task function.
 */
void thread_pool_submit(thread_pool_t *pool, void (*function)(void*), void *argument);

/**
 * @brief Shuts down the thread pool.
 * 
 * @param pool Pointer to the thread pool to be shut down.
 */
void thread_pool_shutdown(thread_pool_t *pool);

#endif /* THREAD_POOL_H_ */
