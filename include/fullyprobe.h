#ifndef FP_H
#define FP_H

//#include <set>

//using std::make_tuple;
//using std::get;
#include "set.h"
#include "linesolve.h"
#include "cdef.h"
#include "board.h"

class FullyProbe {
    public:
        Board gp[25][25][2]; 
        Board max_g0 , max_g1;
        int method;

        myset P; //將未上色的pixel放入list P中 
				myset oldP;

				// test
				Board mainBoard;
				double eigen[25][25];
				void clear(){ MEMSET_ZERO(eigen); }
};

double choose( int method , double mp1 , double mp0 ); 
int fp2( FullyProbe& , LineSolve& , Board& );
int probe( FullyProbe& , LineSolve& , Board& , int , int ); //fp1
int probeG( FullyProbe&  , LineSolve& ,  int , int , uint64_t ); //fp2
void setBestPixel( FullyProbe& , Board& );

#endif
