#include <cstring.h>
#include <cmem.h>
#include <cgc.h>
#include <string.h>
#include <stdarg.h>

#define islngstr(len) (len > SHRSTRLENGTH)
#define HASH_PRIME 16777619u
#define HASH_OFFSET_BASIS 2166136261u

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

static hash_t hashf(const char *data, size_t len) {
  hash_t hash = HASH_OFFSET_BASIS;
  for (size_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= HASH_PRIME;
  }
  return hash;
}

static String *search_string(cyth_State *C, const char *data, size_t len, hash_t hash) {
  String *str;
  if (islngstr(len))
    return NULL;
  for (cmem_t i = 0; i < C->cache.nstrings; i++) {
    str = C->cache.strings[i];
    if (str->len == len) {
      if (str->lng)
        return (memcmp(str->data, data, len) == 0) ? str : NULL;
      else if (str->hash == hash) /* not lngstr, there is a hash */
        return (memcmp(str->data, data, len) == 0) ? str : NULL;
    }
  }
  return NULL;
}

static String *create_strobj(cyth_State *C,
                             const char *data, size_t len,
                             hash_t hash, sbyte lng) {
  gc_object *ref = cythG_newobj(C, GCOS);
  String *s = &ref->v.s;
  s->data = cythM_malloc(C, len + 1);
  s->len = len;
  s->hash = hash;
  s->lng = lng;
  s->aux = -1;
  memcpy(s->data, data, len);
  s->data[len] = 0;
  return s;
}

/* create a string and search for it in cache */
String *cythS_new(cyth_State *C, char *data) {
  size_t len = strlen(data);
  String *s;
  hash_t hash;
  if (islngstr(len)) hash = 0;
  else hash = hashf(data, len);
  s = search_string(C, data, len, hash);
  if (s) return s;
  s = create_strobj(C, data, len, hash, islngstr(len));
  if (s->lng) return s; /* do not put it in the cache */
  if (C->cache.nstrings >= C->cache.stringsize)
    cythM_vecgrow(C, C->cache.strings, C->cache.stringsize, String);
  C->cache.strings[C->cache.nstrings++] = s;
  return s;
}

int cythS_streq(String *s1, String *s2) {
  if (s1->len == s2->len) {
    if (s1->lng || s2->lng) return memcmp(s1->data, s2->data, s1->len);
    else if (s1->hash == s2->hash) return s1 == s2;
    else return 0;
  }
  return 0;
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
  if (!s->lng) cythS_erase(C, s);
  cythM_free(C, s->data, s->len+1);
}

/*
** remove string from string table
** warning: this does not free the string pointer
*/
void cythS_erase(cyth_State *C, String *s) {
  stringtable *cache = &C->cache;
  for (cmem_t i = 0; i < cache->nstrings; i++) {
    if (cythS_streq(s, cache->strings[i])) {
      for (cmem_t j = i; j < cache->nstrings - 1; j++)
        cache->strings[j] = cache->strings[j+1];
      cache->nstrings--;
      return;
    }
  }
  return;
}
