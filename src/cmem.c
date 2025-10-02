#include <cmem.h>
#include <cstate.h>
#include <cgc.h>

/*
** For memory errors, we can't directly use
** cythE_error as it allocates memory for strings
** also, this excludes the recover point thing.
*/
#define cythM_rawmemerr(C, ...) \
  {fprintf(stderr, "[Error]: Memory error: "); \
  fprintf(stderr, __VA_ARGS__); \
  cythE_closestate(C); exit(1);}

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
  if (G->count >= GCTHRESHOLD) {
    cythG_full(C);
    return _realloc(ptr, size);
  }
  return NULL;
}

void cythM_error(cyth_State *C, const char *type, cmem_t size) {
  if (type != NULL) {
    cythM_rawmemerr(C,
      "Block of %s and size %lu is too big.\n", type, size);
  } else {
    cythM_rawmemerr(C,
      "Block of size %lu is too big.\n", size);
  }
}

/* helper function to reallocate vectors */
void cythM_grow(cyth_State *C, void **ptr,
                cmem_t *size, cmem_t scalar, const char *type) {
  cmem_t old_size = *size;
  *size *= 2;
  void *tmp = _realloc(*ptr, (*size)*scalar);
  if (tmp == NULL) {
    *size = old_size + INCSIZE;
    tmp = _realloc(*ptr, (*size)*scalar);
    if (tmp == NULL)
      tmp = tryagain(C, *ptr, (*size)*scalar);
    if (tmp == NULL)
      cythM_error(C, type, *size);
  }
  C->G->count += *size - old_size;
  C->G->total += *size - old_size;
  *ptr = tmp;
}

void *cythM_malloc(cyth_State *C, cmem_t size) {
  if (size == 0) return NULL;
  void *ptr = _realloc(NULL, size);
  if (ptr == NULL)
    ptr = tryagain(C, ptr, size);
  if (ptr == NULL)
    cythM_error(C, NULL, size);
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
  return _realloc(ptr, size);
}

void cythM_free(cyth_State *C, void *ptr, cmem_t size) {
  C->G->count -= size;
  _realloc(ptr, 0);
  ptr = NULL;
}
