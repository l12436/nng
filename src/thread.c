#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX 100

typedef struct {
	int id;
	int max;
	int sum;
} data_t;

typedef struct {
	int ID;
	pthread_t *thread;
	data_t *ret;
} threadInfo;

typedef struct {
	threadInfo info[MAX];
	int used;
} pool_t;

static pool_t pool = {0};
static pthread_mutex_t mutex;

void *threadFunction(void *msg) {
	int i = 0;
	data_t *data = ( data_t*)msg;
	
	if (msg == NULL)  {
		return NULL;
	}
	
	pthread_mutex_lock(&mutex);
	printf("job %d start\n", data->id);
	for (i = 0; i < data->max; ++i) {
		data->sum += i;
	}
	printf("job %d done\n", data->id);
	pthread_mutex_unlock(&mutex);
	
	return msg;
}

void threadJoin(int threadCount) {
	int i = 0;
	
	for (i = 0; i < threadCount; ++i) {
		pthread_join(*pool.info[i].thread, (void**)&pool.info[i].ret);
	}
}

void getThreadResult(int threadCount) {
	int i = 0;
	for (i = 0; i < threadCount; ++i) {
		printf("thread %d: %d\n", pool.info[i].ret->id, pool.info[i].ret->sum);
	}
	
}

void addThread(int id, data_t *data) {
	if (pool.used >= MAX) {
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
		}
	}
}

int thread_example() {
	data_t data[2];
	int i = 0;
	int threadCount = 2;
	
	memset(&data[i], 0, sizeof(data_t) * 2);
	data[0].max = 10;
	data[0].id = 0;
	data[1].max = 100;
	data[0].id = 1;
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		printf("mutex error\n");
		return 1;
	}
	
	for (i = 0; i < threadCount; ++i) {
		addThread(i, &data[i]);
	}
	threadJoin(threadCount);
	getThreadResult(threadCount);
	
	pthread_mutex_destroy(&mutex);
	freeThread();
	return 0;
}
