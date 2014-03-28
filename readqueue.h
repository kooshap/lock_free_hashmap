/**
 * Multithreaded work queue.
 * Copyright (c) 2012 Ronald Bennett Cemer
 * This software is licensed under the BSD license.
 * See the accompanying LICENSE.txt for details.
 */

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <pthread.h>

typedef struct worker {
	pthread_t thread;
	int terminate;
	struct readqueue *readqueue;
	struct worker *prev;
	struct worker *next;
} worker_t;

typedef struct job {
	void (*job_function)(struct job *job);
	void *user_data;
	struct job *prev;
	struct job *next;
} job_t;

typedef struct readqueue {
	struct worker *workers;
	struct job *waiting_jobs;
	pthread_mutex_t jobs_mutex;
	pthread_cond_t jobs_cond;
} readqueue_t;

int readqueue_init(readqueue_t *readqueue, int numWorkers);

void readqueue_shutdown(readqueue_t *readqueue);

void readqueue_add_job(readqueue_t *readqueue, job_t *job);

#endif	/* #ifndef WORKQUEUE_H */
