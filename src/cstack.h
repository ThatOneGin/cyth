#ifndef CSTACK_H
#define CSTACK_H
#include <cstate.h>

int cythA_push(cyth_State *C, Tvalue v);
Tvalue cythA_pop(cyth_State *C);
int cythA_pushint(cyth_State *C, int i);
int cythA_popint(cyth_State *C);
#endif
