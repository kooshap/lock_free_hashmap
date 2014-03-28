/**
 * Multithreaded work queue.
 * Copyright (c) 2012 Ronald Bennett Cemer
 * This software is licensed under the BSD license.
 * See the accompanying LICENSE.txt for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "readqueue.h"

#define LL_ADD(item, list) { \
	item->prev = NULL; \
	item->next = list; \
	list = item; \
}

#define LL_REMOVE(item, list) { \
	if (item->prev != NULL) item->prev->next = item->next; \
	if (item->next != NULL) item->next->prev = item->prev; \
	if (list == item) list = item->next; \
	item->prev = item->next = NULL; \
}

static void *worker_function(void *ptr) {
	worker_t *worker = (worker_t *)ptr;
	job_t *job;

	while (1) {
		/* Wait until we get notified. */
		pthread_mutex_lock(&worker->readqueue->jobs_mutex);
		while (worker->readqueue->waiting_jobs == NULL) {
			/* If we're supposed to terminate, break out of our continuous loop. */
			if (worker->terminate) break;

			pthread_cond_wait(&worker->readqueue->jobs_cond, &worker->readqueue->jobs_mutex);
		}

		/* If we're supposed to terminate, break out of our continuous loop. */
		if (worker->terminate) break;

		job = worker->readqueue->waiting_jobs;
		if (job != NULL) {
			LL_REMOVE(job, worker->readqueue->waiting_jobs);
		}
		pthread_mutex_unlock(&worker->readqueue->jobs_mutex);

		/* If we didn't get a job, then there's nothing to do at this time. */
		if (job == NULL) continue;

		/* Execute the job. */
		job->job_function(job);
	}

	free(worker);
	pthread_exit(NULL);
}

int readqueue_init(readqueue_t *readqueue, int numWorkers) {
	int i;
	worker_t *worker;
	pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;

	if (numWorkers < 1) numWorkers = 1;
	memset(readqueue, 0, sizeof(*readqueue));
	memcpy(&readqueue->jobs_mutex, &blank_mutex, sizeof(readqueue->jobs_mutex));
	memcpy(&readqueue->jobs_cond, &blank_cond, sizeof(readqueue->jobs_cond));

	for (i = 0; i < numWorkers; i++) {
		if ((worker = malloc(sizeof(worker_t))) == NULL) {
			perror("Failed to allocate all workers");
			return 1;
		}
		memset(worker, 0, sizeof(*worker));
		worker->readqueue = readqueue;
		if (pthread_create(&worker->thread, NULL, worker_function, (void *)worker)) {
			perror("Failed to start all worker threads");
			free(worker);
			return 1;
		}
		LL_ADD(worker, worker->readqueue->workers);
	}

	return 0;
}

void readqueue_shutdown(readqueue_t *readqueue) {
	worker_t *worker = NULL;

	/* Set all workers to terminate. */
	for (worker = readqueue->workers; worker != NULL; worker = worker->next) {
		worker->terminate = 1;
	}

	/* Remove all workers and jobs from the work queue.
	 * wake up all workers so that they will terminate. */
	pthread_mutex_lock(&readqueue->jobs_mutex);
	readqueue->workers = NULL;
	readqueue->waiting_jobs = NULL;
	pthread_cond_broadcast(&readqueue->jobs_cond);
	pthread_mutex_unlock(&readqueue->jobs_mutex);
}

void readqueue_add_job(readqueue_t *readqueue, job_t *job) {
	/* Add the job to the job queue, and notify a worker. */
	pthread_mutex_lock(&readqueue->jobs_mutex);
	LL_ADD(job, readqueue->waiting_jobs);
	pthread_cond_signal(&readqueue->jobs_cond);
	pthread_mutex_unlock(&readqueue->jobs_mutex);
}
