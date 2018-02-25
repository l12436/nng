#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <pthread.h> 
 
#include "probsolver.h"
#include "options.h"

using namespace std;

/*
 * startClock is from program start solving
 * thisClock is from current problem start solving
 *
 * clock_t is high resolution but overflow after 3600s
 * time_t support bigger range but only count by seconds
 */
 
int result;  //將回傳值設成void 
void *NonogramSolverWrapper(void *solver)
{
 int probData;
 NonogramSolver *x_solver = (NonogramSolver *)solver;
 result = x_solver->doSolve(&probData);
 return (void *) &result;
}

void *thread_fun1(void *arg)
{
 
  NonogramSolverWrapper;
 
  pthread_exit(NULL);
}
 
void *thread_fun2(void *arg)
{
 
  NonogramSolverWrapper;

  pthread_exit(NULL);
}

void *thread_fun3(void *arg)
{
 
  NonogramSolverWrapper;

  pthread_exit(NULL);
}

void *thread_fun4(void *arg)
{
 
  NonogramSolverWrapper;

  pthread_exit(NULL);
}

void writePerDuration(const Options& option, int probN, time_t startTime, clock_t thisClock , clock_t startClock )
{
		if(!option.simple)
		{
			printf ( "$%3d\ttime:%4lfs\ttotal:%ld\n" , probN , (double)(clock()-thisClock)/CLOCKS_PER_SEC, time(NULL)-startTime);
		}
		else
		{
			if( probN%100==0 )
				printf("%3d\t%4ld\t%11.6lf\n",probN,time(NULL)-startTime,(double)(clock()-startClock)/CLOCKS_PER_SEC);
		}

		if(option.keeplog)
		{
			FILE* log = fopen( option.logFileName , "a+" );
			fprintf ( log , "%3d\t\t%11.6lf\n" , probN
					, (double)(clock()-thisClock)/CLOCKS_PER_SEC);
			fclose(log);
		}
}

void writeTotalDuration(const Options& option, time_t startTime, clock_t startClk )
{
	printf("Total:\n%4ld\t%11.6lf\n",time(NULL)-startTime,(double)(clock()-startClk)/CLOCKS_PER_SEC);
	if(option.keeplog)
	{
		FILE* log = fopen( option.logFileName , "a+" );
		fprintf(log,"Total:\n%4ld\t%11.6lf\n",time(NULL)-startTime,(double)(clock()-startClk)/CLOCKS_PER_SEC);
		fclose(log);
	}
}

int main(int argc , char *argv[])
{
	Options option;
	if(!option.readOptions(argc, argv))
	{
		printf("\nAborted: Illegal Options.\n");
		return 0;
	}

	if(option.genLogFile())
	{
		printf("\nopen log(%s) and write info failed\n",option.logFileName);
		return 0;
	}

	clearFile(option.outputFileName);

	int *inputData;
	int probData[50*14];
	inputData = allocMem(1001*50*14);
	readFile(option.inputFileName,inputData);

	time_t startTime = time(NULL);
	clock_t startClk = clock();
	clock_t thisClk;
    
	NonogramSolver nngSolver;   
	nngSolver.setMethod(option.method);
	initialHash();

	vector<Board> answer;
	answer.resize(option.problemEnd-option.problemStart+1);
	pthread_t thread1,thread2,thread3,thread4;
	for( int probN = option.problemStart ; probN <= option.problemEnd ; ++probN )
	{
		thisClk = clock();

		getData( inputData ,probN ,probData ); 
	   
	    pthread_t thread1,thread2,thread3,thread4; 
	    
        pthread_create(&thread1, NULL, thread_fun1, &nngSolver);  // 執行緒1
        pthread_create(&thread2, NULL, thread_fun2, &nngSolver);  // 執行緒2
        pthread_create(&thread3, NULL, thread_fun3, &nngSolver);  // 執行緒3
        pthread_create(&thread4, NULL, thread_fun4, &nngSolver);  // 執行緒4
		pthread_join(thread1,NULL);
        pthread_join(thread2,NULL);
        pthread_join(thread3,NULL);
        pthread_join(thread4,NULL);
		 
	    if( !nngSolver.doSolve(probData) ){}
		Board ans = nngSolver.getSolvedBoard();   

		if( option.selfCheck && !checkAns(ans, probData) )
		{
			printf("Fatal Error: Answer not correct\n");
			return 1;
		}

		answer[probN-option.problemStart] = ans;
  
		writePerDuration(option,probN,startTime,thisClk,startClk); //一題一題印 
		printBoard(option.outputFileName, answer[probN-1], probN);
	}
	delete[] inputData;

	//for( int probN = option.problemStart, i = 0 ; probN <= option.problemEnd ; ++probN, ++i )
		//printBoard(option.outputFileName, answer[i], probN);

	writeTotalDuration(option,startTime,startClk);

	return 0;
}


