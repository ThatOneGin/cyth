#include <cmem.h>
#include <cstate.h>
#include <cgc.h>

/* auxiliary function */
static void *_realloc(void *ptr, cmem_t size) {
  if (ptr == NULL && size > 0) {
    return malloc(size);
  } else if (ptr != NULL && size == 0) {
    free(ptr);
  } else if (ptr != NULL && size > 0) {
    return realloc(ptr, size);
  }
  return NULL;
}

/*
** If an allocation fails, try calling GC to free
** some memory and try again.
*/
static void *tryagain(cyth_State *C, void *ptr, cmem_t size) {
  global_State *G = C->G;
  if (G->count >= G->threshold) {
    cythG_full(C);
    return _realloc(ptr, size);
  }
  return NULL;
}

void cythM_error(cyth_State *C) {
  cyth_writeerror(MEMERRMSG);
  cythE_closestate(C);
  exit(1);
}

/* helper function to reallocate vectors */
void cythM_grow(cyth_State *C, void **ptr,
                cmem_t *size, cmem_t scalar) {
  cmem_t old_size = *size;
  *size *= 2;
  if (*size == 0)
    *size = INCSIZE;
  void *tmp = _realloc(*ptr, (*size)*scalar);
  if (tmp == NULL) {
    *size = old_size + INCSIZE;
    tmp = _realloc(*ptr, (*size)*scalar);
    if (tmp == NULL)
      tmp = tryagain(C, *ptr, (*size)*scalar);
    if (tmp == NULL)
      cythM_error(C);
  }
  C->G->count += scalar * (*size - old_size);
  C->G->total += scalar * (*size - old_size);
  *ptr = tmp;
}

void *cythM_malloc(cyth_State *C, cmem_t size) {
  if (size == 0) return NULL;
  void *ptr = _realloc(NULL, size);
  if (ptr == NULL)
    ptr = tryagain(C, ptr, size);
  if (ptr == NULL)
    cythM_error(C);
  C->G->count += size;
  C->G->total += size;
  return ptr;
}

void *cythM_realloc(cyth_State *C, void *ptr, cmem_t oldsize, cmem_t size) {
  if (size == 0)
    C->G->count -= oldsize;
  else if (size < oldsize) {
    C->G->count -= (oldsize - size);
  } else {
    cmem_t count = size - oldsize;
    C->G->count += count;
    C->G->total += count;
  }
  void *p = _realloc(ptr, size);
  if (!p)
    p = tryagain(C, ptr, size);
  if (!p)
    cythM_error(C);
  return p;
}

void cythM_free(cyth_State *C, void *ptr, cmem_t size) {
  C->G->count -= size;
  _realloc(ptr, 0);
  ptr = NULL;
}
