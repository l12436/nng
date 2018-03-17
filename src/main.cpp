#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <pthread.h>

#include "thread.h"
#include "probsolver.h"
#include "options.h"

#define __USE_THREAD__
#define __BLANACE__

using namespace std;

/*
 * startClock is from program start solving
 * thisClock is from current problem start solving
 *
 * clock_t is high resolution but overflow after 3600s
 * time_t support bigger range but only count by seconds
 */

#ifdef __BLANACE__
typedef struct {
	bool isSolved;
} probData_t;
#endif

typedef struct {
	Options *option;
	int **input;
	int id;
	int32_t total;
	time_t startTime;
	clock_t startClk;
#ifdef __BLANACE__
	probData_t *probDataState;
#endif
	FILE* log;
}data_t;

typedef struct {
	Board board;
	int Id;
} answer_t;

typedef struct {
	clock_t *clk;
	vector<answer_t> answer;
	int totalSolved;
	int total;
} result_t;

pool_t pool;
pthread_mutex_t mutex;
pthread_mutex_t mutexWriteLog;
#ifdef __BLANACE__
pthread_mutex_t mutexIsSolved;
#endif

#ifndef __USE_THREAD__
vector<Board> answer;
#endif

#ifndef __USE_THREAD__
void writePerDuration ( const Options &option, int probN, time_t startTime, clock_t thisClock , clock_t startClock );
#else
void writePerDuration ( const Options &option, int probN, time_t startTime, clock_t thisClock , clock_t startClock, FILE *log );
#endif

void *NonogramSolverWrapper ( void *arg )
{
	int probData[50 * 14];
	data_t *data = (data_t*)arg;
	NonogramSolver solver;
	result_t *result = (result_t*)malloc(sizeof(result_t));
#ifdef __BLANACE__
	clock_t *clk = (clock_t*)malloc(sizeof(clock_t) * (data->option->problemEnd));
#else
	clock_t *clk = (clock_t*)malloc(sizeof(clock_t) * (data->option->problemEnd / data->total));
#endif
	int i = 0;
	
	memset(result, 0, sizeof(result_t));
#ifdef __BLANACE__
	memset(clk, 0, (data->option->problemEnd));
#else
	memset(clk, 0, (data->option->problemEnd / data->total));
#endif
	result->clk = clk;
	solver.setMethod ( data->option->method );
	initialHash();
	
#ifdef __BLANACE__
	result->answer.resize ( (data->option->problemEnd - data->option->problemStart + 1) );
#else
	result->answer.resize ( (data->option->problemEnd - data->option->problemStart + 1) / data->total );
#endif

#ifdef __BLANACE__
	for ( int probN = 1 ; probN <= data->option->problemEnd; ++probN, ++i ) {
#else
	for ( int probN = data->id + 1 ; probN <= data->option->problemEnd; probN+=data->total, ++i ) {
#endif
		result->clk[i] = clock();
#ifdef __BLANACE__
		pthread_mutex_lock(&mutexIsSolved);
		if ( data->probDataState[i].isSolved ) {
			pthread_mutex_unlock(&mutexIsSolved);
			continue;
		}
		else {
			data->probDataState[i].isSolved = true;	
		}
		pthread_mutex_unlock(&mutexIsSolved);
#endif
		getData ( *data->input , probN , probData );

		if ( !solver.doSolve ( probData ) ) {}

		Board ans = solver.getSolvedBoard();

		if ( data->option->selfCheck && !checkAns ( ans, probData ) ) {
			printf ( "Fatal Error: Answer not correct\n" );
			continue;
		}

		result->answer[i].Id = probN;
		result->answer[i].board = ans;
		
		writePerDuration ( *data->option, probN, data->startTime, result->clk[i], data->startClk, data->log ); //一題一題印
	}
	
	result->totalSolved = i;

	return result;
}

#ifndef __USE_THREAD__
void writePerDuration ( const Options &option, int probN, time_t startTime, clock_t thisClock , clock_t startClock )
#else
void writePerDuration ( const Options &option, int probN, time_t startTime, clock_t thisClock , clock_t startClock, FILE *log )
#endif
{
	if ( !option.simple ) {
		printf ( "$%3d\ttime:%4lfs\ttotal:%ld\n" , probN , ( double ) ( clock() - thisClock ) / CLOCKS_PER_SEC, time ( NULL ) - startTime );
	}
	else {
		if ( probN % 100 == 0 )
			printf ( "%3d\t%4ld\t%11.6lf\n", probN, time ( NULL ) - startTime, ( double ) ( clock() - startClock ) / CLOCKS_PER_SEC );
	}
	if ( option.keeplog ) {
#ifdef __USE_THREAD__
		pthread_mutex_lock(&mutexWriteLog);
#else
		FILE *log = fopen ( option.logFileName , "a+" );
#endif
		fprintf ( log , "%3d\t\t%11.6lf\n" , probN
				  , ( double ) ( clock() - thisClock ) / CLOCKS_PER_SEC );
		fflush(log);
#ifdef __USE_THREAD__
		pthread_mutex_unlock(&mutexWriteLog);
#else
		fclose ( log );
#endif
	}
}

void writeTotalDuration ( const Options &option, time_t startTime, clock_t startClk )
{
	printf ( "Total:\n%4ld\t%11.6lf\n", time ( NULL ) - startTime, ( double ) ( clock() - startClk ) / CLOCKS_PER_SEC );

	if ( option.keeplog ) {
		FILE *log = fopen ( option.logFileName , "a+" );
		fprintf ( log, "Total:\n%4ld\t%11.6lf\n", time ( NULL ) - startTime, ( double ) ( clock() - startClk ) / CLOCKS_PER_SEC );
		fclose ( log );
	}
}

int main ( int argc , char *argv[] )
{
	Options option;

	if ( !option.readOptions ( argc, argv ) ) {
		printf ( "\nAborted: Illegal Options.\n" );
		return 0;
	}
	
	strcpy(option.logFileName, "log.txt");
	option.keeplog = true;

	if ( option.genLogFile() ) {
		printf ( "\nopen log(%s) and write info failed\n", option.logFileName );
		return 0;
	}

	clearFile ( option.outputFileName );

	int *inputData;
// 	int probData[50 * 14];
	inputData = allocMem ( 1001 * 50 * 14 );
	readFile ( option.inputFileName, inputData );

	time_t startTime = time ( NULL );
	clock_t startClk = clock();
// 	clock_t thisClk;

	NonogramSolver nngSolver;
	nngSolver.setMethod ( option.method );
	initialHash();

#ifdef __USE_THREAD__
	data_t **datapool;
	FILE *fp = fopen ( option.logFileName , "a+" );
	pool.max = 100;
	int total = 4;
	datapool = (data_t**) malloc(sizeof(data_t*) * total);
	memset(datapool, 0, sizeof(data_t*) * total);
	if (initPool ( total ) < 0) {
		return 1;
	}
#ifdef __USE_THREAD__
	pthread_mutex_init(&mutexWriteLog, NULL);
#endif
#ifdef __BLANACE__
	pthread_mutex_init(&mutexIsSolved, NULL);
	probData_t *probDataState = (probData_t*)malloc(sizeof(probData_t) * option.problemEnd);
	memset(probDataState, 0, sizeof(probData_t) * option.problemEnd);
#endif
	for (int i = 0; i < total; ++i) {
		data_t *data = (data_t*)malloc(sizeof(data_t));
		memset(data, 0, sizeof(data_t));
		data->option = &option;
		data->total = total;
		data->startTime = startTime;
		data->startClk = startClk;
		data->input = &inputData;
		data->id = i;
#ifdef __BLANACE__
		data->probDataState = probDataState;
#endif
		data->log = fp;
		datapool[i] = data;
		addThread(NonogramSolverWrapper, data);
	}
	for (int i = 0; i < total; ++i) {
		waitResult();
	}
	for (int threadid = 0; threadid < total; ++threadid) {
		result_t *res = (result_t*)pool.info[threadid].ret;
		if (res != NULL) {
			for ( int index = 0 ; index < res->totalSolved ; ++index ) {
				printBoard ( option.outputFileName, res->answer[index].board, res->answer[index].Id );
			}
		}
	}
	for ( int i = 0; i < total; ++i ) {
		result_t *res = (result_t*)pool.info[i].ret;
		if (datapool[i] != NULL) {
			free(datapool[i]);
		}
		if (res != NULL) {
			if (res->clk != NULL) {
				free(res->clk);
				res->clk = NULL;
			}
			free(pool.info[i].ret);
			pool.info[i].ret = NULL;
		}
		
	}
	free(datapool);
	datapool = NULL;
#else
	answer.resize ( option.problemEnd - option.problemStart + 1 );

	for ( int probN = option.problemStart ; probN <= option.problemEnd ; ++probN ) {
		thisClk = clock();

		getData ( inputData , probN , probData );

		if ( !nngSolver.doSolve ( probData ) ) {}

		Board ans = nngSolver.getSolvedBoard();

		if ( option.selfCheck && !checkAns ( ans, probData ) ) {
			printf ( "Fatal Error: Answer not correct\n" );
			return 1;
		}

		answer[probN - option.problemStart] = ans;

		writePerDuration ( option, probN, startTime, thisClk, startClk ); //一題一題印
		printBoard ( option.outputFileName, answer[probN - 1], probN );
	}
#endif

	delete[] inputData;

	//for( int probN = option.problemStart, i = 0 ; probN <= option.problemEnd ; ++probN, ++i )
	//printBoard(option.outputFileName, answer[i], probN);

	writeTotalDuration ( option, startTime, startClk );

#ifdef __USE_THREAD__
	freePool();
	pthread_mutex_destroy(&mutexWriteLog);
#endif
#ifdef __BLANACE__
	pthread_mutex_destroy(&mutexIsSolved);
	if (probDataState != NULL)
		free( probDataState );
	fclose(fp);
#endif
	return 0;
}



