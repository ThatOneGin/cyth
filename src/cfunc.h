#ifndef CFUNC_H
#define CFUNC_H
#include <copcode.h>
#include <cobject.h>
#include <limits.h>

#define MAXCODESIZE USHRT_MAX 
#define MAXCONSTSIZE USHRT_MAX

typedef struct {
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
} cyth_Function;

cyth_Function *cyth_newfunc(cyth_State *C);
int cythF_emitC(cyth_Function *f, Instruction i, int line);
int cythF_emitK(cyth_Function *f, Tvalue k);
void cythF_freefunc(cyth_Function *f);
#endif