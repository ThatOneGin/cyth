#ifndef CMEM_H
#define CMEM_H
#include <cstate.h>

/*
** if an allocation of a vector fails,
** instead of doubling the size,
** add INCSIZE to it.
*/
#define INCSIZE 5

#define cythM_vecnew(C, v, s, t) ((v)=cythM_malloc(C, sizeof(t)*(s)))
#define cythM_vecgrow(C, v, s, t) \
  cythM_grow(C, ((void**)&(v)), &s, sizeof(t), #t)
#define cythM_vecfree(C, v, s) (cythM_free(C, v), (s)=0)

void cythM_grow(cyth_State *C, void **ptr, cmem_t *size, cmem_t scalar, const char *type);
void *cythM_malloc(cyth_State *C, cmem_t size);
void *cythM_realloc(cyth_State *C, void *ptr, cmem_t oldsize, cmem_t size);
void cythM_free(cyth_State *C, void *ptr, cmem_t size);

#endif