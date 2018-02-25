#include "fullyprobe.h"
#include "linesolve.h"
#include "board.h"
#include <cstdio>
#include <algorithm>
#include <unistd.h>
using namespace std;

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

int probe( FullyProbe& fp , LineSolve& ls , Board &board , int pX ,int pY )
{

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

int probeG( FullyProbe& fp ,LineSolve& ls ,int pX ,int pY ,uint64_t pVal )
{
	pVal -= BIT_ZERO;
	Board newG = fp.gp[pX][pY][pVal];
	int newGstate = propagate(ls , newG );
	if( newGstate == SOLVED || newGstate == CONFLICT )
		return newGstate;

#ifdef FP2
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

	fp.gp[pX][pY][pVal] = newG;

	return newGstate;
}
