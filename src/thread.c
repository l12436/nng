#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread.h"

typedef struct {
	int id;
	int max;
	int sum;
} data_t;

void *threadFunction(void *msg) {
	int i = 0;
	data_t *data = ( data_t*)msg;
	
	if (msg == NULL)  {
		return NULL;
	}
	
	pthread_mutex_lock(&mutex);
	for (i = 0; i < data->max; ++i) {
		data->sum += i;
	}
	pthread_mutex_unlock(&mutex);
	
	return msg;
}

void getThreadResult(int threadCount) {
	int i = 0;
	
	for (i = 0; i < threadCount; ++i) {
		pthread_join(*pool.info[i].thread, (void**)&pool.info[i].ret);
	}
}

void addThread(void *data) {
	if (pool.used >= pool.max) {
		return;
	}
	pthread_t *thread = (pthread_t*)malloc(sizeof(pthread_t));
	memset(thread, 0, sizeof(pthread_t));
	pthread_create(thread, NULL, threadFunction, data);
	pool.info[pool.used].thread = thread;
	pool.used++;
}

void freeThread() {
	int i = 0;
	for (i = 0; i < pool.used; ++i) {
		if (pool.info[i].thread != NULL) {
			free(pool.info[i].thread);
			pool.info[i].thread = NULL;
		}
	}
}

int initPool(int size)
{
	pool.info = (thread_info_t*)malloc(sizeof(thread_info_t) * size);
	if (pool.info == NULL) {
		return -1;
	}
	memset(pool.info, 0, sizeof(thread_info_t) * size);
	pool.max = size;
}

void freePool()
{
	freeThread();
	free(pool.info);
	pool.info = NULL;
}

static void showResult(int threadCount) {
	int i = 0;
	for (i = 0; i < threadCount; ++i) {
		printf("thread %d: %d\n", ((data_t*)pool.info[i].ret)->id, ((data_t*)pool.info[i].ret)->sum);
	}
	
}

static int thread_example() {
	data_t data[2];
	int i = 0;
	int threadCount = 2;
	
	memset(&data[i], 0, sizeof(data_t) * 2);
	data[0].max = 10;
	data[1].max = 100;
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		printf("mutex error\n");
		return 1;
	}
	
	for (i = 0; i < threadCount; ++i) {
		data[i].id = i;
		addThread(&data[i]);
	}
	getThreadResult(threadCount);
	showResult(threadCount);
	
	pthread_mutex_destroy(&mutex);
	freeThread();
	return 0;
}
