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

using namespace std;

/*
 * startClock is from program start solving
 * thisClock is from current problem start solving
 *
 * clock_t is high resolution but overflow after 3600s
 * time_t support bigger range but only count by seconds
 */

typedef struct {
	Options *option;
	int **input;
	int id;
	int32_t total;
	time_t startTime;
	clock_t startClk;
}data_t;

typedef struct {
	clock_t *clk;
	vector<Board> answer;
	int total;
} result_t;

pool_t pool;
pthread_mutex_t mutex;

#ifndef __USE_THREAD__
vector<Board> answer;
#endif

void writePerDuration ( const Options &option, int probN, time_t startTime, clock_t thisClock , clock_t startClock );

void *NonogramSolverWrapper ( void *arg )
{
	int probData[50 * 14];
	data_t *data = (data_t*)arg;
	NonogramSolver solver;
	result_t *result = (result_t*)malloc(sizeof(result_t));
	clock_t *clk = (clock_t*)malloc(sizeof(clock_t) * (data->option->problemEnd / data->total));
	int i = 0;
	
	memset(result, 0, sizeof(result_t));
	memset(clk, 0, (data->option->problemEnd / data->total));
	result->clk = clk;
	solver.setMethod ( data->option->method );
	initialHash();	
	
	result->answer.resize ( (data->option->problemEnd - data->option->problemStart + 1) / data->total );

	for ( int probN = data->id + 1 ; probN <= data->option->problemEnd; probN+=data->total, ++i ) {
		result->clk[i] = clock();

		getData ( *data->input , probN , probData );

		if ( !solver.doSolve ( probData ) ) {}

		Board ans = solver.getSolvedBoard();

		if ( data->option->selfCheck && !checkAns ( ans, probData ) ) {
			printf ( "Fatal Error: Answer not correct\n" );
			continue;
		}

		result->answer[i] = ans;
		
		writePerDuration ( *data->option, probN, data->startTime, result->clk[i], data->startClk ); //一題一題印
	}

	return result;
}

void writePerDuration ( const Options &option, int probN, time_t startTime, clock_t thisClock , clock_t startClock )
{
	if ( !option.simple ) {
		printf ( "$%3d\ttime:%4lfs\ttotal:%ld\n" , probN , ( double ) ( clock() - thisClock ) / CLOCKS_PER_SEC, time ( NULL ) - startTime );
	}
	else {
		if ( probN % 100 == 0 )
			printf ( "%3d\t%4ld\t%11.6lf\n", probN, time ( NULL ) - startTime, ( double ) ( clock() - startClock ) / CLOCKS_PER_SEC );
	}
#ifndef __USE_THREAD__
	if ( option.keeplog ) {
		FILE *log = fopen ( option.logFileName , "a+" );
		fprintf ( log , "%3d\t\t%11.6lf\n" , probN
				  , ( double ) ( clock() - thisClock ) / CLOCKS_PER_SEC );
		fclose ( log );
	}
#endif
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

	if ( option.genLogFile() ) {
		printf ( "\nopen log(%s) and write info failed\n", option.logFileName );
		return 0;
	}

	clearFile ( option.outputFileName );

	int *inputData;
	int probData[50 * 14];
	inputData = allocMem ( 1001 * 50 * 14 );
	readFile ( option.inputFileName, inputData );

	time_t startTime = time ( NULL );
	clock_t startClk = clock();
	clock_t thisClk;

	NonogramSolver nngSolver;
	nngSolver.setMethod ( option.method );
	initialHash();

#ifdef __USE_THREAD__
	data_t **datapool;
	pool.max = 100;
	int total = 4;
	datapool = (data_t**) malloc(sizeof(data_t*) * total);
	memset(datapool, 0, sizeof(data_t*) * total);
	if (initPool ( total ) < 0) {
		return 1;
	}
	for (int i = 0; i < total; ++i) {
		data_t *data = (data_t*)malloc(sizeof(data_t));
		memset(data, 0, sizeof(data_t));
		data->option = &option;
		data->total = total;
		data->startTime = startTime;
		data->startClk = startClk;
		data->input = &inputData;
		data->id = i;
		datapool[i] = data;
		addThread(NonogramSolverWrapper, data);
	}
	for (int i = 0; i < total; ++i) {
		waitResult();
	}
	for ( int probN = option.problemStart ; probN <= option.problemEnd ; ++probN ) {
		result_t *res = (result_t*)pool.info[probN % total].ret;
		int index = (probN - 1) % total;
		if (res != NULL) {
			printBoard ( option.outputFileName, res->answer[index], probN );
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
#endif
	return 0;
}



