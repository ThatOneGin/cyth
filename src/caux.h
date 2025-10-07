#ifndef CYTH_H
#define CYTH_H
#include <cstate.h>
#include <cio.h>
void cythA_remove(cyth_State *C, int idx);
void cythA_insert(cyth_State *C, int idx);
void cythA_settop(cyth_State *C, int top);
int cythA_push(cyth_State *C, Tvalue v);
Tvalue cythA_pop(cyth_State *C);
int cythA_pushint(cyth_State *C, int i);
int cythA_popint(cyth_State *C);
int cythA_pushstr(cyth_State *C, String *string);
String *cythA_popstr(cyth_State *C);
int cythA_pushlit(cyth_State *C, String *literal);
String *cythA_poplit(cyth_State *C);
int cythA_load(cyth_State *C, Stream *s, char *name);
#endif