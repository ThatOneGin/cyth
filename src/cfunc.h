#ifndef CFUNC_H
#define CFUNC_H
#include <copcode.h>
#include <cobject.h>
#include <limits.h>

enum calltype {
  CYTHCALL,
  CCALL
};

struct cyth_Function {
  cyth_State *C;
  Instruction *code;
  cmem_t ncode;
  cmem_t codesize;
  Tvalue *k; /* constants */
  cmem_t nk;
  cmem_t ksize; 
  int *lineinfo; /* debug info */
  cmem_t nline;
  cmem_t linesize;
  cyth_Function **f;
  cmem_t nf;
  cmem_t fsize;
};

cyth_Function *cythF_newfunc(cyth_State *C);
int cythF_emitC(cyth_Function *f, Instruction i, int line);
int cythF_emitK(cyth_Function *f, Tvalue k);
int cythF_emitF(cyth_Function *f, cyth_Function *f2);
void cythF_freefunc(cyth_Function *f);
void cythF_precall(cyth_State *C, stkrel func, int nargs);
void cythF_poscall(cyth_State *C);
void cythF_call(cyth_State *C, int i, int nargs);
#endif