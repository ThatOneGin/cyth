#include <cstate.h>
#include <cmem.h>
#include <cstring.h>
#include <cgc.h>
#include <caux.h>
#include <stdarg.h>

global_State G = {0};

static void init_stack(cyth_State *C) {
  C->base.p = malloc(sizeof(*C->base.p) * MINSTACK);
  if (C->base.p == NULL)
    cythE_error(C, "Couldn't allocate stack.\n");
  C->top.p = C->base.p;
  C->maxoff = MINSTACK;
}

/* saves the stack offsets */
static void save_stack(cyth_State *C) {
  C->top.offs = calculate_stack_offset(C, C->top.p);
  Call_info *ci = C->ci;
  while (ci != NULL) {
    ci->top.offs = calculate_stack_offset(C, ci->top.p);
    ci->func.offs = calculate_stack_offset(C, ci->func.p);
    ci = ci->prev;
  }
}

/* restore the stack after an reallocation (should be used after save_stack) */
static void restore_stack(cyth_State *C) {
  C->top.p = apply_stack_offset(C, C->top.offs);
  Call_info *ci = C->ci;
  while (ci != NULL) {
    ci->top.p = apply_stack_offset(C, ci->top.offs);
    ci->func.p = apply_stack_offset(C, ci->func.offs);
    ci = ci->prev;
  }
}

/* reallocate stack as a vector */
static void realloc_stack(cyth_State *C) {
  save_stack(C);
  stkrel newbase = realloc(C->base.p, sizeof(*C->base.p)*(C->maxoff*2));
  if (newbase == NULL) {
    restore_stack(C);
    cythE_error(C, "Couldn't reallocate stack.\n");
  } else {
    C->base.p = newbase;
    C->maxoff *= 2;
    restore_stack(C);
  }
}

static inline cmem_t stack_size(cyth_State *C) {
  return C->top.p - C->base.p;
}

/*
** allocate a new cyth_State
** should be paired with cythE_closestate
*/
cyth_State *cythE_openstate(void) {
  static byte main = 1;
  cyth_State *C = malloc(sizeof(cyth_State));
  if (!C) return C;
  init_stack(C);
  C->main = main;
  C->G = &G;
  C->errhandler = NULL;
  C->ci = NULL;
  C->ncalls = 0;
  C->gt = cythH_new(C);
  cythA_push(C, t2obj(C->gt));
  cythS_init(C);
  if (main) main = 0;
  return C;
}

void cythE_newci(cyth_State *C) {
  if (C->ncalls >= MAXCALLS)
    cythE_error(C, "Call stack overflow.\n");
  Call_info *ci = cythM_malloc(C, sizeof(Call_info));
  ci->top.p = NULL;
  ci->func.p = NULL;
  ci->type = 0;
  ci->prev = C->ci;
  C->ci = ci;
  C->ncalls++;
}

cmem_t cythE_gettop(cyth_State *C) {
  return C->top.p - (C->ncalls > 0 ? (C->ci->func.p + 1) : (C->base.p));
}

static void closecalls(cyth_State *C) {
  Call_info *ci = C->ci;
  while (ci != NULL) {
    Call_info *prev = ci->prev;
    cythM_free(C, ci, sizeof(*ci));
    ci = prev;
  }
}

void cythE_closestate(cyth_State *C) {
  free(C->base.p);
  C->top.p = NULL;
  C->base.p = NULL;
  C->maxoff = 0;
  C->errhandler = NULL;
  cythS_clear(C);
  closecalls(C);
  if (C->main) cythG_freeall(C);
  free(C);
  C = NULL;
}

void cythE_error(cyth_State *C,
                 const char *f, ...) {
  char *fmt = s2cstr(cythS_sprintf(C, "[Error]: %s\n", f));
  va_list ap;
  va_start(ap, f);
  String *message = cythS_vsprintf(C, fmt, ap);
  va_end(ap);
  cythE_throw(C, 1, message);
}

void cythE_inctop(cyth_State *C) {
  cmem_t stksz = stack_size(C);
  if (stksz+1 >= C->maxoff)
    realloc_stack(C);
  C->top.p++;
}

void cythE_dectop(cyth_State *C) {
  cmem_t stksz = stack_size(C);
  if (stksz == 0 || (C->ci != NULL &&
                   C->ci->func.p + 1 == C->top.p)) {
    cythE_error(C, "Stack underflow");
  }
  C->top.p--;
}

void cythE_throw(cyth_State *C, byte errcode, String *errmsg) {
  if (C->errhandler != NULL) {
    C->errhandler->errmsg = errmsg;
    C->errhandler->errcode = errcode;
    cythA_pushstr(C, errmsg);
    cythA_pushint(C, 1);
    cyth_throw(C, 1);
  } else {
    printf("%*s", (unsigned int)errmsg->len, errmsg->data);
    cythE_closestate(C);
    exit(1);
  }
}

byte cythE_runprotected(cyth_State *C,
                        cyth_Pfunction f,
                        void *ud) {
  cyth_jmpbuf newhandler;
  newhandler.previous = C->errhandler;
  C->errhandler = &newhandler;
  cyth_try(C, f, ud);
  C->errhandler = C->errhandler->previous;
  return newhandler.errcode;
}
