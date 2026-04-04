#include <cstate.h>
#include <cstring.h>
#include <cparser.h>
#include <cstate.h>
#include <string.h>
#include <cmem.h>
#include <cgc.h>
#include <cvm.h>
#include <caux.h>

#define checkidx(idx) if (idx > 0) idx = -idx;
#define auxcheck(C, e, msg) ((!(e)) ? cythE_error(C, "%s: %s", #e, msg) : ((void)0))


/* remove value at position idx */
void cythA_remove(cyth_State *C, int idx) {
  checkidx(idx);
  stkrel sv = &C->top.p[idx];
  for (;sv != C->top.p-1; sv++) {
    objcopy(sv, sv+1);
  }
  C->top.p--;
}

/* insert top-1 at position idx */
void cythA_insert(cyth_State *C, int idx) {
  checkidx(idx);
  if (idx < (C->base.p - C->top.p)) return;
  stkrel sv = &C->top.p[idx];
  Tvalue val = C->top.p[-1];
  for (; sv < C->top.p-1; sv++) {
    objcopy(sv+1, sv);
  }
  C->top.p[idx] = val;
}

static void fillstack(stkrel from, stkrel to, Tvalue with) {
  while (from < to) {
    *from = with;
    from++;
  }
}

static void expect_top_type(cyth_State *C, int t) {
  if (cyth_tt(C->top.p-1) != t)
    cythE_error(C, "Expected %s, but got %s",
      cythA_type2str(t),
      cythA_type2str(cyth_tt(C->top.p-1)));
}

/*
** set the stack slot top to 'top'. if the stack
** is being reallocated, fill the new entries to
** none (no value).
*/
void cythA_settop(cyth_State *C, int top) {
  if (top > 0) { /* absolute index */
    if ((unsigned)top > C->maxoff) {
      cmem_t savedtop = C->top.p - C->base.p;
      stkrel newbase = realloc(C->base.p, sizeof(*C->base.p)*(C->maxoff+top));
      if (newbase == NULL)
        cythE_error(C, "Couldn't reallocate stack.\n");
      C->maxoff += top;
      C->base.p = newbase;
      C->top.p = C->base.p + (savedtop + top);
      fillstack(C->base.p+savedtop, C->top.p, NONE);
    } else {
      stkrel oldtop = C->top.p;
      C->top.p += top;
      fillstack(oldtop, C->top.p, NONE);
    }
  } else { /* relative index */
    if (top == 0) return;
    else if ((C->base.p - C->top.p) > top)
      cythE_error(C, "Invalid stack top index.\n");
    C->top.p += top;
  }
}

void cythA_push(cyth_State *C, Tvalue v) {
  *C->top.p = v;
  cythE_inctop(C);
}

Tvalue cythA_pop(cyth_State *C) {
  cythE_dectop(C);
  return *C->top.p;
}

void cythA_pushint(cyth_State *C, int i) {
  Tvalue v = i2obj(i);
  *C->top.p = v;
  cythE_inctop(C);
}

int cythA_popint(cyth_State *C) {
  expect_top_type(C, CYTH_INTEGER);
  cythE_dectop(C);
  return obj2i(C->top.p);
}

void cythA_pushstr(cyth_State *C, String *string) {
  Tvalue v = s2obj(string);
  *C->top.p = v;
  cythE_inctop(C);
}

String *cythA_popstr(cyth_State *C) {
  expect_top_type(C, CYTH_STRING);
  cythE_dectop(C);
  return obj2s(C->top.p);
}

/* parse stream with a recover point set */
static int pparse(cyth_State *C, void *aux) {
  Stream *s = (Stream*)aux;
  char *name = s2cstr(cythA_popstr(C));
  stkrel top = C->top.p;
  cyth_Function *f = cythP_parse(C, s, name);
  if (f == NULL) {
    /*
    ** message is on the stack
    ** so we push the error int
    */
    cythA_pushint(C, 1);
  } else {
    C->top.p = top; /* erase lexer table */
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

/* create an userdata object and push it onto the stack */
void *cythA_udnew(cyth_State *C, cmem_t n) {
  gc_object *ref;
  userdata ud = {0};
  ud.destructor = NULL;
  ud.type = UDVAL;
  ud.u.val.data = cythM_malloc(C, n);
  ud.u.val.size = n;
  ref = cythG_newobj(C, GCOU);
  ud.u.val.ref = ref;
  ref->v.u = ud;
  cythA_push(C, ud2obj(ud));
  return ud.u.val.data;
}

/* set the destruct method of userdata at top-i */
void cythA_udsetdestructor(cyth_State *C, int i, cyth_Destructor d) {
  checkidx(i);
  if (cyth_tt(&C->top.p[i]) != CYTH_USERDATA) return;
  else {
    /* only values can have destructors */
    if (obj2ud(&C->top.p[i]).type != UDVAL) return;
    userdata ud = obj2ud(&C->top.p[i]);
    ud.destructor = d;
    gc_object *ref = ud.u.val.ref;
    ref->v.u.destructor = d;
  }
}

/* put function 'u' as 'name' in global table */
void cythA_regcf(cyth_State *C, cyth_Cfunction f, char *name) {
  userdata u;
  u.destructor = NULL;
  u.type = UDFUN;
  u.u.cfunc = f;
  cythV_setglobal(C, cythS_new(C, name), ud2obj(u));
}

char *cythA_type2str(int i) {
  switch (i) {
#define X(name, str, value) case name: return str;
  VALUES
#undef X
  default:
    assert(0);
    break;
  }
}

/* only works for C functions */
Tvalue cythA_arg(cyth_State *C, int idx) {
  auxcheck(C, idx > 0, "Invalid argument index");
  return *cythE_peek(C, idx);
}
