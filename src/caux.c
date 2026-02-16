#include <cstate.h>
#include <cstring.h>
#include <cparser.h>
#include <cstate.h>
#include <string.h>

#define checkidx(idx) if (idx > 0) idx = -idx;

/* remove value at position idx */
void cythA_remove(cyth_State *C, int idx) {
  checkidx(idx);
  stkrel sv = &C->top[idx];
  for (;sv != C->top-1; sv++) {
    objcopy(sv, sv+1);
  }
  C->top--;
}

/* insert top-1 at position idx */
void cythA_insert(cyth_State *C, int idx) {
  checkidx(idx);
  if (idx < (C->base - C->top)) return;
  stkrel sv = &C->top[idx];
  Tvalue val = C->top[-1];
  for (; sv < C->top-1; sv++) {
    objcopy(sv+1, sv);
  }
  C->top[idx] = val;
}

static void fillstack(stkrel from, stkrel to, Tvalue with) {
  while (from < to) {
    *from = with;
    from++;
  }
}

/*
** set the stack slot top to 'top'. if the stack
** is being reallocated, fill the new entries to
** none (no value).
*/
void cythA_settop(cyth_State *C, int top) {
  if (top > 0) { /* absolute index */
    if ((unsigned)top > C->maxoff) {
      cmem_t savedtop = C->top - C->base;
      stkrel newbase = realloc(C->base, sizeof(*C->base)*(C->maxoff+top));
      if (newbase == NULL)
        cythE_error(C, "Couldn't reallocate stack.\n");
      C->maxoff += top;
      C->base = newbase;
      C->top = C->base + (savedtop + top);
      fillstack(C->base+savedtop, C->top, NONE);
    } else {
      stkrel oldtop = C->top;
      C->top += top;
      fillstack(oldtop, C->top, NONE);
    }
  } else { /* relative index */
    if (top == 0) return;
    else if ((C->base - C->top) > top)
      cythE_error(C, "Invalid stack top index.\n");
    C->top += top;
  }
}

int cythA_push(cyth_State *C, Tvalue v) {
  int top = (int)cythE_gettop(C);
  *C->top = v;
  cythE_inctop(C);
  return top;
}

Tvalue cythA_pop(cyth_State *C) {
  cythE_dectop(C);
  return *C->top;
}

int cythA_pushint(cyth_State *C, int i) {
  Tvalue v = i2obj(i);
  int top = (int)cythE_gettop(C);
  *C->top = v;
  cythE_inctop(C);
  return top;
}

int cythA_popint(cyth_State *C) {
  cythE_dectop(C);
  if (cyth_tt(C->top) != CYTH_INTEGER) return 0;
  else return obj2i(C->top);
}

int cythA_pushstr(cyth_State *C, String *string) {
  Tvalue v = s2obj(string);
  int top = (int)cythE_gettop(C);
  *C->top = v;
  cythE_inctop(C);
  return top;
}

String *cythA_popstr(cyth_State *C) {
  cythE_dectop(C);
  if (cyth_tt(C->top) != CYTH_STRING) return NULL;
  else return obj2s(C->top);
}

/* parse stream with a recover point set */
static int pparse(cyth_State *C, void *aux) {
  Stream *s = (Stream*)aux;
  char *name = s2cstr(cythA_popstr(C));
  stkrel top = C->top;
  cyth_Function *f = cythP_parse(C, s, name);
  if (f == NULL) {
    /*
    ** message is on the stack
    ** so we push the error int
    */
    cythA_pushint(C, 1);
  } else {
    C->top = top; /* erase lexer table */
    cythA_push(C, f2obj(f));
    cythA_pushint(C, 0);
  }
  return 0;
}

int cythA_load(cyth_State *C, Stream *s, char *name) {
  String *sname = cythS_new(C, name);
  cythA_pushstr(C, sname);
  cythE_runprotected(C, pparse, s);
  return cythA_popint(C);
}
