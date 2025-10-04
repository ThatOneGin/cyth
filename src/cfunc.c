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

void cythF_precall(cyth_State *C, stkrel func, int nargs) {
  cythE_newci(C);
  Call_info *ci = C->ci;
  ci->func = func;
  ci->type =
    (cyth_tt(func) == CYTH_USERDATA &&
    (obj2ud(func).type == UDFUN)) ? CCALL : CYTHCALL;
  ci->top = func+nargs+1;
  if (ci->type == CCALL) {
    ci->u.c.nargs = nargs;
  } else {
    ci->u.cyth.f = obj2f(func);
    ci->u.cyth.pc = 0;
  }
  C->top = ci->top;
}

void cythF_poscall(cyth_State *C) {
  Call_info *ci = C->ci->prev;
  cythM_free(C, C->ci, sizeof(*C->ci));
  C->ci = ci;
  C->ncalls--;
}

void cythF_call(cyth_State *C, int i, int nargs) {
  stkrel func = &C->top[i];
  cythF_precall(C, func, nargs);
  Call_info *ci = C->ci;
  if (ci->type == CYTHCALL) {
    cythV_exec(C, C->ci);
  } else {
    obj2ud(ci->func).u.cfunc(C);
  }
  cythF_poscall(C);
}