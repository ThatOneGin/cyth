#ifndef CGC_H
#define CGC_H
#include <cobject.h>
#include <cstate.h>

#define GCTHRESHOLD 1000000
#define GCMAXSWEEP 20

void cythG_full(cyth_State *C);
cmem_t cythG_total(cyth_State *C);
cmem_t cythG_inuse(cyth_State *C);
void cythG_freeall(cyth_State *C);
gc_object *cythG_newobj(cyth_State *C, byte tt_);
gc_object *cythG_uncoll(cyth_State *C, byte tt_);
#endif
