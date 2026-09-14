#include <cstring.h>
#include <cmem.h>
#include <cgc.h>
#include <string.h>
#include <stdarg.h>

/* initialize cyth_State cache */
void cythS_init(cyth_State *C) {
  C->cache.nstrings = 0;
  C->cache.stringsize = INCSIZE;
  cythM_vecnew(C, C->cache.strings, C->cache.stringsize, String);
}

/* clear cyth_State cache */
void cythS_clear(cyth_State *C) {
  /* GC will take care of the strings */
  cythM_vecfree(C, C->cache.strings, C->cache.stringsize, String);
}

/* create a string and search for it in cache */
String *cythS_new(cyth_State *C, char *data) {
  size_t len = strlen(data);
  String *s;
  gc_object *ref;
   for (cmem_t i = 0; i < C->cache.nstrings; i++) {
     if (len == C->cache.strings[i]->len &&
         memcmp(C->cache.strings[i]->data, data, len) == 0) {
       return C->cache.strings[i];
     }
  }
  ref = cythG_newobj(C, GCOS);
  s = &ref->v.s;
  s->data = cythM_malloc(C, len+1);
  s->len = len;
  s->aux = -1;
  s->data[s->len] = 0;
  memcpy(s->data, data, s->len);
  if (C->cache.nstrings >= C->cache.stringsize)
    cythM_vecgrow(C, C->cache.strings, C->cache.stringsize, String);
  C->cache.strings[C->cache.nstrings++] = s;
  return s;
}

int cythS_streq(String *s1, String *s2) {
  return s1 == s2;
}

String *cythS_vsprintf(cyth_State *C,
                       const char *f,
                       va_list ap) {
  char buffer[BUFFERSIZE];
  vsnprintf(buffer, BUFFERSIZE, f, ap);
  return cythS_new(C, buffer);
}

String *cythS_sprintf(cyth_State *C, const char *f, ...) {
  va_list ap;
  va_start(ap, f);
  String *s = cythS_vsprintf(C, f, ap);
  va_end(ap);
  return s;
}

/*
** erase string from the string table and
** free its memory
*/
void cythS_free(cyth_State *C, String *s) {
  cythS_erase(C, s);
  cythM_free(C, s->data, s->len+1);
}

/*
** remove string from string table
** warning: this does not free the string pointer
*/
void cythS_erase(cyth_State *C, String *s) {
  stringtable *cache = &C->cache;
  for (cmem_t i = 0; i < cache->nstrings; i++) {
    if (s == cache->strings[i]) {
      for (cmem_t j = i; j < cache->nstrings - 1; j++)
        cache->strings[j] = cache->strings[j+1];
      cache->nstrings--;
      return;
    }
  }
  return;
}
