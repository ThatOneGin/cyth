#include <cstate.h>
#include <cstring.h>
#include <cparser.h>
#include <cstate.h>
#include <string.h>
#include <cmem.h>
#include <cgc.h>
#include <cvm.h>
#include <caux.h>

#define check(C, e, msg) \
  if (!(e))              \
    cythE_error(C, msg)

static void expect_top_type(cyth_State *C, int t) {
  if (cyth_tt(C->top.p-1) != t)
    cythE_error(C, "Expected %s, but got %s",
      cythA_type2str(t),
      cythA_type2str(cyth_tt(C->top.p-1)));
}

void cythA_settop(cyth_State *C, int i) {
  Call_info *ci = C->ci;
  stkrel top, base = NULL;
  ptrdiff_t off = 0;
  if (ci) {
    top = ci->top.p;
    base = ci->func.p + 1;
  } else {
    top = C->top.p;
    base = C->base.p;
  }
  if (i < 0) {
    check(C, ~i <= top - base, "invalid new top");
    off = i + 1;
  } else {
    cmem_t diff = top - base;
    check(C, diff + i <= C->maxoff - diff, "invalid new top");
    off = i;
    for (ptrdiff_t i = 0; i < off; i++)
      top[i] = NONE;
  }
  if (ci) ci->top.p = top + off;
  else C->top.p = top + off;
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
#define _streq(e1, e2) (strcmp(e1, e2) == 0)

static int pparse(cyth_State *C, void *aux) {
  Stream *s = (Stream*)aux;
  stkrel top = C->top.p;
  cyth_Function *f = NULL;
  char *name = s2cstr(cythA_popstr(C));
  char *ext = strrchr(name, '.');
  if (!ext)
    cythE_error(C, "no file extension for file '%s'", name);
  if (_streq(ext, ".cyth")) f = cythP_parse_cyth(C, s, name);
  else if (_streq(ext, ".cx")) f = cythP_parse_cx(C, s, name);
  else cythE_error(C, "unknown file extension for %s", name);
  if (f == NULL) {
    /* message is already on the stack */
    cythA_pushint(C, 1);
  } else {
    C->top.p = top; /* erase lexer table */
    cythA_push(C, f2obj(f));
    cythA_pushint(C, 0);
  }
  return 0;
}

#undef _streq

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
  ud.data = cythM_malloc(C, n);
  ud.size = n;
  ref = cythG_newobj(C, GCOU);
  ud.ref = ref;
  ref->v.u = ud;
  cythA_push(C, ud2obj(ud));
  return ud.data;
}

/* set the destruct method of userdata at top-i */
void cythA_udsetdestructor(cyth_State *C, int i, cyth_Destructor d) {
  if (cyth_isuserdata(C, i)) {
    userdata ud = obj2ud(cythE_peek(C, i));
    ud.destructor = d;
    gc_object *ref = ud.ref;
    ref->v.u.destructor = d;
  }
}

/* put function 'u' as 'name' in global table */
void cythA_regcf(cyth_State *C, cyth_Cfunction f, char *name) {
  cythV_setglobal(C, cythS_new(C, name), cf2obj(f));
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

int cythA_typeof(cyth_State *C, int i) {
  return cyth_tt(cythE_peek(C, i));
}

/* only works for C functions */
Tvalue cythA_arg(cyth_State *C, int idx) {
  check(C, idx > 0, "invalid argument index (less than zero)");
  return *cythE_peek(C, idx);
}

void cythA_newlib(cyth_State *C, cyth_reg *funcs) {
  String *s;
  cyth_Cfunction f;
  Table *t = cythH_new(C);
  int i = 0;
  cythA_push(C, t2obj(t));
  while (funcs[i].func != NULL &&
         funcs[i].name != NULL) {
    s = cythS_new(C, funcs[i].name);
    f = funcs[i].func;
    cythH_append(C, t, s2obj(s), cf2obj(f));
    i++;
  }
}