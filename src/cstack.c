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
