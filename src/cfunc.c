#include <cfunc.h>
#include <cmem.h>
#include <cvm.h>

cyth_Function *cythF_newfunc(cyth_State *C) {
  cyth_Function *f = cythM_malloc(C, sizeof(cyth_Function));
  f->C = C;
  f->code = NULL;
  f->ncode = 0;
  f->codesize = 0;
  f->k = NULL;
  f->nk = 0;
  f->ksize = 0;
  f->lineinfo = NULL;
  f->nline = 0;
  f->linesize = 0;
  return f;
}

/* Emit instruction i with debug info */
int cythF_emitC(cyth_Function *f, Instruction i, int line) {
  if (f->ncode >= f->codesize)
    cythM_vecgrow(f->C, f->code, f->codesize, Instruction);
  if (f->nline >= f->linesize)
    cythM_vecgrow(f->C, f->lineinfo, f->linesize, int);
  f->code[f->ncode++] = i;
  f->lineinfo[f->nline++] = line;
  return f->ncode-1;
}

/* emit constant value */
int cythF_emitK(cyth_Function *f, Tvalue k) {
  for (cmem_t i = 0; i < f->nk; i++) {
    /* reuse constants if possible */
    if (cythV_objequ(f->C, f->k[i], k)) {
      return i;
    }
  }
  if (f->nk >= f->ksize)
    cythM_vecgrow(f->C, f->k, f->ksize, Tvalue);
  f->k[f->nk++] = k;
  return f->nk-1;
}

void cythF_freefunc(cyth_Function *f) {
  cythM_vecfree(f->C, f->code, f->codesize, Instruction);
  cythM_vecfree(f->C, f->k, f->ksize, Tvalue);
  cythM_vecfree(f->C, f->lineinfo, f->linesize, int);
  cythM_free(f->C, f, sizeof(cyth_Function));
}
