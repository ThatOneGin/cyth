#include <cstate.h>
#include <cmem.h>
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
  if (main) main = 0;
  return C;
}

cmem_t cythE_gettop(cyth_State *C) {
  return C->top - C->base;
}

void cythE_closestate(cyth_State *C) {
  cmem_t top = cythE_gettop(C);
  cythM_free(C, C->base, sizeof(C->base)*top);
  C->top = NULL;
  C->base = NULL;
  C->maxoff = 0;
  if (C->main) cythG_freeall(C);
  free(C);
  C = NULL;
}

void cythE_error(cyth_State *C,
                 const char *f, ...) {
  cythE_closestate(C);
  fprintf(stderr, "[Error]: ");
  va_list ap;
  va_start(ap, f);
  vfprintf(stderr, f, ap);
  va_end(ap);
  exit(1);
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
