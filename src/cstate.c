#include <cstate.h>
#include <cmem.h>
#include <cstring.h>
#include <stdarg.h>
#include <cgc.h>

global_State G = {0};

static void init_stack(cyth_State *C) {
  C->base = malloc(sizeof(*C->base) * MINSTACK);
  if (C->base == NULL)
    cythE_error(C, "Couldn't allocate stack.\n");
  C->top = C->base;
  C->maxoff = MINSTACK;
}

/* reallocate stack as a vector */
static void realloc_stack(cyth_State *C) {
  cmem_t savedtop = C->top - C->base;
  stkrel newbase = realloc(C->base, sizeof(*C->base)*(C->maxoff*2));
  if (newbase == NULL)
    cythE_error(C, "Couldn't reallocate stack.\n");
  C->maxoff *= 2;
  C->base = newbase;
  C->top = C->base + savedtop;
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
  cythS_init(C);
  if (main) main = 0;
  return C;
}

void cythE_newci(cyth_State *C) {
  if (C->ncalls >= MAXCALLS)
    cythE_error(C, "Stack overflow.\n");
  Call_info *ci = cythM_malloc(C, sizeof(Call_info));
  ci->top = NULL;
  ci->func = NULL;
  ci->type = 0;
  ci->prev = C->ci;
  C->ci = ci;
  C->ncalls++;
}

cmem_t cythE_gettop(cyth_State *C) {
  return C->top - C->base;
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
  cmem_t top = cythE_gettop(C);
  cythM_free(C, C->base, sizeof(*C->base)*top);
  C->top = NULL;
  C->base = NULL;
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
  cmem_t top = cythE_gettop(C);
  if (top+1 >= C->maxoff)
    realloc_stack(C);
  C->top++;
}

void cythE_dectop(cyth_State *C) {
  cmem_t top = cythE_gettop(C);
  if (top == 0)
    cythE_error(C, "Stack underflow.\n");
  C->top--;
}

void cythE_throw(cyth_State *C, byte errcode, String *errmsg) {
  if (C->errhandler != NULL) {
    C->errhandler->errmsg = errmsg;
    C->errhandler->errcode = errcode;
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
