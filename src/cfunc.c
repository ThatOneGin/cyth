#include <cfunc.h>
#include <cmem.h>
#include <cvm.h>
#include <cgc.h>

#define checkveclimit(C, lim, ty) \
  if ((lim) > MAXVECSIZE) \
    cythE_error(C, \
      "Maximum size of vector" \
      " reached (%u elements, type %s)", MAXVECSIZE, #ty);

cyth_Function *cythF_newfunc(cyth_State *C) {
  gc_object *ref = cythG_newobj(C, GCOF);
  cyth_Function *f = cythM_malloc(C, sizeof(cyth_Function));
  ref->v.f = f;
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
  f->f = NULL;
  f->nf = 0;
  f->fsize = 0;
  return f;
}

/* Emit instruction i with debug info */
int cythF_emitC(cyth_Function *f, Instruction i, int line) {
  checkveclimit(f->C, f->codesize, Instruction);
  if (f->ncode >= f->codesize)
    cythM_vecgrow(f->C, f->code, f->codesize, Instruction);
  checkveclimit(f->C, f->codesize, Instruction);
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
  checkveclimit(f->C, f->ksize, Tvalue);
  if (f->nk >= f->ksize)
    cythM_vecgrow(f->C, f->k, f->ksize, Tvalue);
  f->k[f->nk++] = k;
  return f->nk-1;
}

int cythF_emitF(cyth_Function *f, cyth_Function *f2) {
  checkveclimit(f->C, f->fsize, cyth_Function);
  if (f->nf >= f->fsize)
    cythM_vecgrow(f->C, f->f, f->fsize, sizeof(*f->f));
  f->f[f->nf++] = f2;
  return f->nf-1;
}

void cythF_freefunc(cyth_Function *f) {
  cythM_vecfree(f->C, f->code, f->codesize, Instruction);
  cythM_vecfree(f->C, f->k, f->ksize, Tvalue);
  cythM_vecfree(f->C, f->lineinfo, f->linesize, int);
  cythM_vecfree(f->C, f->f, f->fsize, cyth_Function*);
  cythM_free(f->C, f, sizeof(cyth_Function));
}

void cythF_precall(cyth_State *C, stkrel func, int nargs) {
  if (cyth_tt(func) == CYTH_USERDATA) {
    if (obj2ud(func).type != UDFUN) {
      cythE_error(C,
        "Trying to call an userdata "
        "value that is not a function");
    }
  } else if (cyth_tt(func) != CYTH_FUNCTION &&
             cyth_tt(func) != CYTH_USERDATA) {
    cythE_error(C, "Trying to call an invalid value.\n");
  }
  cythE_newci(C);
  Call_info *ci = C->ci;
  ci->func = func;
  ci->top = func+nargs+1;
  ci->type = (cyth_tt(func) == CYTH_USERDATA) ? CCALL : CYTHCALL;
  if (ci->type == CCALL) {
    ci->u.c.nargs = nargs;
  } else {
    ci->u.cyth.f = obj2f(func);
    ci->u.cyth.pc = 0;
    ci->u.cyth.locvars = cythH_new(C);
  }
  C->top = ci->top;
}

void cythF_poscall(cyth_State *C) {
  Call_info *ci = C->ci->prev;
  cythM_free(C, C->ci, sizeof(*C->ci));
  C->ci = ci;
  C->ncalls--;
  cythG_full(C); /* collect unused values */
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