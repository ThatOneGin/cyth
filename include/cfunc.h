#ifndef CFUNC_H
#define CFUNC_H
#include <copcode.h>
#include <cobject.h>
#include <limits.h>

#define MAXRESULT 200
#define MAXPARAMS 200

enum calltype {
  CYTHCALL,
  CCALL
};

struct cyth_Function {
  cyth_State *C;
  cmem_t ncode;
  cmem_t nk;
  cmem_t nline;
  cmem_t nf;
  cmem_t codesize;
  cmem_t ksize; 
  cmem_t linesize;
  cmem_t fsize;
  byte nresults;
  byte nparams;
  String *name; /* for debug purposes */
  Instruction *code;
  int *lineinfo; /* debug info */
  Tvalue *k; /* constants */
  cyth_Function **f;
};

cyth_Function *cythF_newfunc(cyth_State *C);
int cythF_emitC(cyth_Function *f, Instruction i, int line);
int cythF_emitK(cyth_Function *f, Tvalue k);
int cythF_emitF(cyth_Function *f, cyth_Function *f2);
void cythF_freefunc(cyth_Function *f);
void cythF_precall(cyth_State *C, stkrel func, int nargs, int nwanted);
void cythF_poscall(cyth_State *C, int nresults, int nwanted);
void cythF_call(cyth_State *C, int i, int nargs, int nresults);
#endif