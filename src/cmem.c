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

static void *tryagain(cyth_State *C, void *ptr, cmem_t size) {
  global_State *G = C->G;
  if (G->count >= GCTHRESHOLD) {
    cythG_full(C);
    return _realloc(ptr, size);
  }
  return NULL;
}

void cythM_error(cyth_State *C, const char *type, cmem_t size) {
  if (type != NULL)
    cythE_error(C,
      "Block of %s and size %lu is too big.\n", type, size);
  else
    cythE_error(C,
      "Block of size %lu is too big.\n", size);
}

void cythM_grow(cyth_State *C, void **ptr,
                cmem_t *size, cmem_t scalar, const char *type) {
  cmem_t old_size = *size;
  *size *= 2;
  void *tmp = _realloc(*ptr, (*size)*scalar);
  if (tmp == NULL) {
    *size = old_size + INCSIZE;
    tmp = _realloc(*ptr, (*size)*scalar);
    if (tmp == NULL)
      tmp = tryagain(C, ptr, (*size)*scalar);
    if (tmp == NULL)
      cythM_error(C, type, *size);
  }
  *ptr = tmp;
}

void *cythM_malloc(cyth_State *C, cmem_t size) {
  if (size == 0) return NULL;
  void *ptr = _realloc(NULL, size);
  if (ptr == NULL)
    cythM_error(C, NULL, size);
  return ptr;
}

void *cythM_realloc(cyth_State *C, void *ptr, cmem_t oldsize, cmem_t size) {
  if (size == 0)
    C->G->count -= oldsize;
  else {
    cmem_t count = size - oldsize;
    C->G->count += count;
  }
  return _realloc(ptr, size);
}

void cythM_free(cyth_State *C, void *ptr, cmem_t size) {
  C->G->count -= size;
  _realloc(ptr, 0);
  ptr = NULL;
}
