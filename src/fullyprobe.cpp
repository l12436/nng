#include "fullyprobe.h"
#include "linesolve.h"
#include "board.h"
#include <cstdio>
#include <algorithm>
#include <unistd.h>
using namespace std;

#define __USE_THREAD_PROBE__

#ifdef __USE_THREAD_PROBE__
#include "thread.h"

extern pool_t pool;
#endif

int fp2 ( FullyProbe& fp , LineSolve& ls , Board& board )
{

	int res = propagate( ls , board); //先呼叫propagate 
	if( res != INCOMP ) //未解完 
		return res;

	Dual_for(i,j) //座標25*25 
		fp.gp[i][j][0] = fp.gp[i][j][1] = board; 

	Dual_for(i,j)
		if( getBit( board,i,j ) == BIT_UNKNOWN ) //檢查哪一點尚未填色 
		{
			fp.P.insert( i*25+j ); //放入list中 
			setBit( fp.gp[i][j][0], i,j,BIT_ZERO ); //填0 
			setBit( fp.gp[i][j][1], i,j,BIT_ONE ); //填1 
		}


	while(1)
	{
		int p = fp.P.begin(); //從list中拿出P 
		if( p == -1 )
		{
			if( fp.oldP.isEmpty() ) //全解完 
				break;  
			else
			{
				fp.P = fp.oldP;  
				fp.oldP.clear();
				continue;
			}
		}

		if( getBit(board,p/25,p%25)==BIT_UNKNOWN ) //往前推得i,j座標位置 
		{
			res = probe( fp , ls , board , p/25 , p%25);
			if( res == SOLVED || res == CONFLICT )
				return res;
		}
	}

	getSize(board);
	//usleep(100000);
	//system("clear");
	//debugBoard(board);
	

	fp.mainBoard = board; //更新盤面 
	setBestPixel( fp , board );

	return INCOMP;
}


void setBestPixel( FullyProbe& fp , Board& board )
{
	//auto max = make_tuple(0,0,0);
	int max[3]={0,0,0};
	double maxPixel = -9E10;

	Dual_for(i,j)
		if( getBit(board,i,j) == BIT_UNKNOWN )
		{
			for( int k = 0 ; k < 50 ; ++k )
			{
				fp.gp[i][j][0].data[k] &= board.data[k];
				fp.gp[i][j][1].data[k] &= board.data[k];
			}

			getSize(fp.gp[i][j][1]);
			getSize(fp.gp[i][j][0]);

//#define AAAA
#ifdef AAAA
			double maxEigen = 0;
		  Dual_for(i,j)
			{
				if( fp.eigen[i][j] <= 1 )
					fp.eigen[i][j] = 1;
				if( fp.eigen[i][j] > maxEigen )
					maxEigen = fp.eigen[i][j];
			}
			Dual_for(i,j)
			{
				if( getBit(fp.gp[i][j][1],i,j) != getBit(fp.mainBoard,i,j) )
					fp.gp[i][j][1].size *= (2-fp.eigen[i][j]/maxEigen)/2;
				if( getBit(fp.gp[i][j][0],i,j) != getBit(fp.mainBoard,i,j) )
					fp.gp[i][j][0].size *= (2-fp.eigen[i][j]/maxEigen)/2;
			}
#endif

			double ch = choose( fp.method , 
						fp.gp[i][j][1].size-board.size ,
						fp.gp[i][j][0].size-board.size );
			

			if( ch > maxPixel ) 
			{
				//max = make_tuple(i,j, fp.gp[i][j][0].size > fp.gp[i][j][1].size ? 0 : 1);
				max[0] = i;
				max[1] = j;
				max[2] = fp.gp[i][j][0].size > fp.gp[i][j][1].size ? 0 : 0;
				maxPixel = ch;
			}
		} // big if end

	//printf("select %d %d %lf\n" , get<0>(max) , get<1>(max) , maxPixel );
	fp.max_g0 = fp.gp[max[0]][max[1]][max[2]];
	fp.max_g1 = fp.gp[max[0]][max[1]][!max[2]];

#ifdef AAAA
	Dual_for(i,j)
	{
		if( getBit(fp.max_g0,i,j) != getBit(fp.mainBoard,i,j) )
			fp.eigen[i][j]++;
		if( getBit(fp.max_g1,i,j) != getBit(fp.mainBoard,i,j) )
			fp.eigen[i][j]++;
	}
#endif
}

#define vlog(x) (log(x+1)+1)
double choose( int method , double mp1 , double mp0 )
{
	//if(mp1<=0)mp1=0;
	//if(mp0<=0)mp0=0;
	//printf("mp1=%d mp0=%d\n",mp1,mp0);
	switch(method)
	{
		case CH_SUM:
			return mp1+mp0;
			break;
		case CH_MIN:
			return min(mp1,mp0);
			break;
		case CH_MAX:
			return max(mp1,mp0);
			break;
		case CH_MUL:
			return ++mp1 * ++mp0;
			break;
		case CH_SQRT:
			return min(mp1,mp0)+sqrt( max(mp1,mp0)/(min(mp1,mp0)+1));
			break;
		case CH_MIN_LOGM:
			return min(mp1,mp0)+vlog(mp1)*vlog(mp0);
			break;
		case CH_MIN_LOGD:
			return min(mp1,mp0)+abs(vlog(mp1)-vlog(mp0));
			break;
		default:
			return ++mp1*++mp0;
	}
}

#ifdef __USE_THREAD_PROBE__
typedef struct {
	int pX;
	int pY;
	int ret;
	FullyProbe *fp;
	LineSolve *ls;
	Board *board;
} probe_data_t;

void *probe0(void *arg) {
	int pX = 0;
	int pY = 0;
	FullyProbe *fp = NULL;
	LineSolve *ls = NULL;
	Board *board = NULL;
	probe_data_t *data = (probe_data_t*)arg;
	int *ret = (int*)malloc(sizeof(int));
	
	pX = data->pX;
	pY = data->pY;
	fp = data->fp;
	ls = data->ls;
	board = data->board;

	for( int i = 0 ; i < 50 ; ++i )
	{
		fp->gp[pX][pY][0].data[i] &= board->data[i];
	}

	int p0 = probeG( *fp ,*ls ,pX ,pY ,BIT_ZERO );
	*ret = p0;
	return ret;
}

void *probe1(void *arg) {
	int pX = 0;
	int pY = 0;
	FullyProbe *fp = NULL;
	LineSolve *ls = NULL;
	Board *board = NULL;
	probe_data_t *data = (probe_data_t*)arg;
	int *ret = (int*)malloc(sizeof(int));

	pX = data->pX;
	pY = data->pY;
	fp = data->fp;
	ls = data->ls;
	board = data->board;

	for( int i = 0 ; i < 50 ; ++i )
	{
		fp->gp[pX][pY][1].data[i] &= board->data[i];
	}

	int p1 = probeG( *fp ,*ls ,pX ,pY ,BIT_ONE );
	*ret = p1;
	return ret;
}
#endif

int probe( FullyProbe& fp , LineSolve& ls , Board &board , int pX ,int pY )
{

#ifdef __USE_THREAD_PROBE__
	int total = 2;
	int p0 = 0;
	int p1 = 0;
	
	if (initPool(total)) {
		return INCOMP;
	}

	probe_data_t **data = (probe_data_t**)malloc(sizeof(probe_data_t*) * total);
	for (int i = 0; i < total; ++i) {
		probe_data_t *d = (probe_data_t*)malloc(sizeof(probe_data_t));
		data[i] = d;
		data[i]->board = &board;
		data[i]->fp = &fp;
		data[i]->ls = &ls;
		data[i]->pX = pX;
		data[i]->pY = pY;
	}
	addThread(probe0, data[0]);
	addThread(probe1, data[1]);
	
	for (int i = 0; i < total; ++i) {
		waitResult();
	}
	p0 = *((int *)pool.info[0].ret);
	p1 = *((int *)pool.info[1].ret);
	for (int i = 0; i < total; ++i) {
		if (data[i] != NULL) {
			free(data[i]);
		}
		free(pool.info[i].ret);
	}
	
	free(data);
	freePool();
#else
	for( int i = 0 ; i < 50 ; ++i )
	{
		fp.gp[pX][pY][0].data[i] &= board.data[i];
		fp.gp[pX][pY][1].data[i] &= board.data[i];
	}

	int p0 = probeG( fp ,ls ,pX ,pY ,BIT_ZERO );
	if( p0 == SOLVED )
	{
		return SOLVED;
	}
	int p1 = probeG( fp ,ls ,pX ,pY ,BIT_ONE );
	if( p1 == SOLVED )
	{
		return SOLVED;
	}
#endif

	if( p0==CONFLICT && p1==CONFLICT )
	{
		return CONFLICT;
	}
	else if( p1==CONFLICT )
	{
		board = fp.gp[pX][pY][0];
	}
	else if( p0==CONFLICT )
	{
		board = fp.gp[pX][pY][1];
	}
	else
	{
		for ( int i = 0 ; i < 50 ; ++i )
			board.data[i] = fp.gp[pX][pY][0].data[i] | fp.gp[pX][pY][1].data[i];
	}

	return INCOMP;
}

#ifdef __USE_THREAD_FP__

pthread_mutex_t mutexIsSolved;
pthread_mutex_t mutexOldP;

typedef struct {
	bool isSolved;
} probData_t;

typedef struct {
	Board *newG;
	FullyProbe *fp;
	probData_t *status;
	int id;
	int pX;
	int pY;
	uint64_t pVal;
} fp_data_t;

void* thread_probeG (void *arg) {
	int pX = 0;
	int pY = 0;
	uint64_t pVal = 0;
	Board *newG = NULL;
	FullyProbe *fp = NULL;

	fp_data_t *data = (fp_data_t*)arg;
	newG = data->newG;
	fp = data->fp;
	pX = data->pX;
	pY = data->pY;
	pVal = data->pVal;

	for( int _x = data->id ; _x < 25 ; ++_x )
	{
		pthread_mutex_lock(&mutexIsSolved);
		if ( data->status[_x].isSolved ) {
			pthread_mutex_unlock(&mutexIsSolved);
			continue;
		}
		else {
			data->status[_x].isSolved = true;	
		}
		pthread_mutex_unlock(&mutexIsSolved);

		uint64_t tmp = newG->data[_x] ^fp->gp[pX][pY][pVal].data[_x];
		if( !tmp )
			continue;
		
		int pos = 0;

		while( 0 != (pos=__builtin_ffsll(tmp)) )
		{
			pos--;
			tmp &= tmp-1;
			int _y = pos>>1,
				_v = pos&1;

			if(_x!=pX&&_y!=pY)
			{
				pthread_mutex_lock(&mutexOldP);
				fp->oldP.insert( _x*25 + _y );
				pthread_mutex_unlock(&mutexOldP);
				setBit( fp->gp[_x][_y][_v] , pX , pY , ( !(pVal==0) ? BIT_ZERO : BIT_ONE ) );
			}
		}
	}
}
#endif

int probeG( FullyProbe& fp ,LineSolve& ls ,int pX ,int pY ,uint64_t pVal )
{
	pVal -= BIT_ZERO;
	Board newG = fp.gp[pX][pY][pVal];
	int newGstate = propagate(ls , newG );
	if( newGstate == SOLVED || newGstate == CONFLICT )
		return newGstate;

#ifdef FP2
#ifdef __USE_THREAD_FP__
	int total = 4;
	pool.max = 100;
	if (initPool(total) < 0) {
		return CONFLICT;
	}
	fp_data_t **datapool = (fp_data_t**) malloc(sizeof(fp_data_t*) * total);
	memset( datapool, 0, sizeof(fp_data_t*) * total);
	probData_t *status = (probData_t*)malloc(sizeof(probData_t) * 2 * 25 * 25);
	memset(status, 0, sizeof(probData_t) * 2 * 25 * 25);
	pthread_mutex_init(&mutexIsSolved, NULL);
	pthread_mutex_init(&mutexOldP, NULL);
	
	for (int i = 0; i < total; ++i) {
		fp_data_t* data = (fp_data_t*)malloc(sizeof(fp_data_t));
		memset(data, 0, sizeof(fp_data_t));
		data->id = i;
		data->status = status;
		data->newG = &newG;
		data->fp = &fp;
		data->pX = pX;
		data->pY = pY;
		data->pVal = pVal;
		datapool[i] = data;
		addThread(thread_probeG, data);
	}

	for (int i = 0; i < total; ++i) {
		waitResult();
	}

	for (int i = 0; i < total; ++i) {
		if (datapool[i] != NULL) {
			free(datapool[i]);
		}
	}
	free(datapool);
	free(status);
	pthread_mutex_destroy(&mutexIsSolved);
	pthread_mutex_destroy(&mutexOldP);
	freePool();
#else
	for( int _x = 0 ; _x < 25 ; ++_x )
	{
		uint64_t tmp = newG.data[_x] ^fp.gp[pX][pY][pVal].data[_x];
		if( !tmp )
			continue;

		int pos = 0;

		while( 0 != (pos=__builtin_ffsll(tmp)) )
		{
			pos--;
			tmp &= tmp-1;
			int _y = pos>>1,
				_v = pos&1;

			if(_x!=pX&&_y!=pY)
			{
				fp.oldP.insert( _x*25 + _y );
				setBit( fp.gp[_x][_y][_v] , pX , pY , ( !(pVal==0) ? BIT_ZERO : BIT_ONE ) );
			}
		}
	}
#endif
#endif

	fp.gp[pX][pY][pVal] = newG;

	return newGstate;
}
