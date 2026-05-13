#ifndef CVM_H
#define CVM_H
#include <cobject.h>
#include <cfunc.h>
#include <cstate.h>

enum BINOPR {
  OPR_ADD,
  OPR_SUB,
  OPR_MUL,
  OPR_DIV,
  OPR_COUNT
};

int cythV_objequ(cyth_State *C, Tvalue t1, Tvalue t2);
int cythV_toboolean(Tvalue v, Tvalue *res);
void cythV_setglobal(cyth_State *C, String *name, Tvalue v);
int cythV_getglobal(cyth_State *C, String *name, Tvalue *res);
void cythV_dup(cyth_State *C);
void cythV_swap(cyth_State *C);
int cythV_arith(cyth_State *C, Tvalue *res, Tvalue l, Tvalue r, int op);
void cythV_exec(cyth_State *C, Call_info *ci);
#endif
