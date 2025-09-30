#include <cstring.h>
#include <cmem.h>
#include <cgc.h>
#include <string.h>

void cythS_init(cyth_State *C) {
  C->cache.nstrings = 0;
  C->cache.stringsize = INCSIZE;
  cythM_vecnew(C, C->cache.strings, C->cache.stringsize, String*);
}

int cythS_delete(cyth_State *C, String *s) {
  for (cmem_t i = 0; i < C->cache.nstrings; i++) {
    if (s == C->cache.strings[i]) {
      return i;
    }
  }
  return -1;
}

void cythS_clear(cyth_State *C) {
  /* GC will take care of the strings */
  cythM_vecfree(C, C->cache.strings, C->cache.stringsize, String*);
}

String *cythS_new(cyth_State *C, char *data) {
  for (cmem_t i = 0; i < C->cache.nstrings; i++) {
    if (strcmp(C->cache.strings[i]->data, data) == 0) {
      return C->cache.strings[i];
    }
  }
  size_t len =  strlen(data);
  String *s = cythM_malloc(C, sizeof(String));
  s->data = cythM_malloc(C, len+1);
  s->len = len;
  snprintf(s->data, s->len+1, "%s", data);
  gc_object *ref = cythG_newobj(C, GCOS);
  ref->v.s = s;
  if (C->cache.nstrings >= C->cache.stringsize)
    cythM_vecgrow(C, C->cache.strings, C->cache.stringsize, String*);
  C->cache.strings[C->cache.nstrings++] = s;
  return s;
}

int cythS_streq(String *s1, String *s2) {
  return s1 == s2;
}
