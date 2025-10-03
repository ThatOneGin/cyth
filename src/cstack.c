#include <cstack.h>
#include <cstate.h>
#include <string.h>

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

int cythA_pushlit(cyth_State *C, String *literal) {
  Tvalue v = l2obj(literal);
  int top = (int)cythE_gettop(C);
  *C->top = v;
  cythE_inctop(C);
  return top;
}

String *cythA_poplit(cyth_State *C) {
  cythE_dectop(C);
  if (cyth_tt(C->top) != CYTH_LITERAL) return NULL;
  else return obj2l(C->top);
}
