#include <cmem.h>
#include <cstate.h>

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

void *cythM_realloc(cyth_State *C, void *ptr, cmem_t size) {
  (void)C; /* will be needed later */
  return _realloc(ptr, size);
}

void cythM_free(cyth_State *C, void *ptr) {
  (void)C; /* will be needed later */
  _realloc(ptr, 0);
  ptr = NULL;
}
