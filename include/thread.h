#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

#define MAX_THREAD 100

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int ID;
	pthread_t *thread;
	void *ret;
} thread_info_t;

typedef struct {
	thread_info_t *info;
	int used;
	int max;
} pool_t;

void waitResult();
void addThread(void *(*func) (void *), void *data);
void freeThread();
int initPool(int size);
void freePool();

#ifdef __cplusplus
}
#endif

#endif
