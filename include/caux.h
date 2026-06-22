#ifndef CYTH_H
#define CYTH_H
#include <cstate.h>
#include <cio.h>

#define cyth_isnone(C, i) (cythA_typeof(C, i) == CYTH_NONE)
#define cyth_isint(C, i) (cythA_typeof(C, i) == CYTH_INTEGER)
#define cyth_isstring(C, i) (cythA_typeof(C, i) == CYTH_STRING)
#define cyth_istable(C, i) (cythA_typeof(C, i) == CYTH_TABLE)
#define cyth_isfunction(C, i) (cythA_typeof(C, i) == CYTH_FUNCTION)
#define cyth_isuserdata(C, i) (cythA_typeof(C, i) == CYTH_USERDATA)
#define cyth_isbool(C, i) (cythA_typeof(C, i) == CYTH_BOOL)
#define cyth_isarray(C, i) (cythA_typeof(C, i) == CYTH_ARRAY)

typedef struct {
  char *name;
  cyth_Cfunction func;
} cyth_reg;

void cythA_remove(cyth_State *C, int idx);
void cythA_insert(cyth_State *C, int idx);
void cythA_settop(cyth_State *C, int top);
void cythA_push(cyth_State *C, Tvalue v);
Tvalue cythA_pop(cyth_State *C);
void cythA_pushint(cyth_State *C, int i);
int cythA_popint(cyth_State *C);
void cythA_pushstr(cyth_State *C, String *string);
String *cythA_popstr(cyth_State *C);
int cythA_load(cyth_State *C, Stream *s, char *name);
void *cythA_udnew(cyth_State *C, cmem_t n);
void cythA_udsetdestructor(cyth_State *C, int i, cyth_Destructor d);
void cythA_regcf(cyth_State *C, cyth_Cfunction f, char *name);
char *cythA_type2str(int i);
int cythA_typeof(cyth_State *C, int i);
Tvalue cythA_arg(cyth_State *C, int idx);
void cythA_newlib(cyth_State *C, cyth_reg *funcs);
#endif
