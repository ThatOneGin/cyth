#ifndef CVM_H
#define CVM_H
#include <cobject.h>
#include <cfunc.h>
#include <cstate.h>

int cythV_objequ(cyth_State *C, Tvalue t1, Tvalue t2);
void cythV_exec(cyth_State *C, Call_info *ci);
#endif