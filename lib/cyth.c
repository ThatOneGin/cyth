/* C standard */

#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* Cyth */

#include <cstate.h>
#include <cmem.h>
#include <cobject.h>
#include <caux.h>
#include <cstring.h>
#include <cchunk.h>
#include <cbuiltin.h>
#include <cvm.h>

/* library */

#define CYTH_INTERNAL
#include "cyth.h"

#define incindex(idx, i) ((idx) < 0 ? ((idx) - (i)) : (idx))

CYTH_API cyth_State *cyth_newstate(void) {
  return cythE_openstate();
}

CYTH_API void cyth_openstdlib(cyth_State *C) {
  cythB_openlib(C);
}

CYTH_API int cyth_gettop(cyth_State *C) {
  return cythE_gettop(C);
}

CYTH_API void cyth_destroystate(cyth_State *C) {
  if (C)
    cythE_closestate(C);
}

CYTH_API void cyth_over(cyth_State *C, int i) {
  Tvalue t;
  t = *cythE_peek(C, i);
  cythA_push(C, t);
}

CYTH_API void cyth_rot(cyth_State *C, int i) {
  stkrel t = cythE_peek(C, -1);
  stkrel s = cythE_peek(C, i);
  Tvalue tmp;
  for (; s <= t; t--, s++) {
    tmp = *s;
    *s = *t;
    *t = tmp;
  }
}

CYTH_API void cyth_pop(cyth_State *C) {
  cythE_dectop(C);
}

static void checktype(cyth_State *C, int t, int i)
{
  int type = cythE_peek(C, i)->tt_;
  if (type != t)
    cythE_error(C, "bad value at index %d (%s expected, got %s)",
      i, cythA_type2str(t), cythA_type2str(type));
}

static void checkarg(cyth_State *C, int t, int i) {
  if (i <= 0)
    cythE_error(C, "invalid argument index %d", i);
  checktype(C, t, i);
}

CYTH_API void cyth_pushinteger(cyth_State *C, cyth_integer i) {
  cythA_push(C, i2obj(i));
}

CYTH_API void cyth_pushstring(cyth_State *C, const char *s)
{
  String *cs = cythS_new(C, (char*)s);
  cythA_push(C, s2obj(cs));
}

CYTH_API void cyth_pushboolean(cyth_State *C, int b)
{
  cythA_push(C, b2obj(b));
}

CYTH_API void cyth_pushcfunction(cyth_State *C, cyth_Cfunction f)
{
  cythA_push(C, cf2obj(f));
}

CYTH_API int cyth_argnone(cyth_State *C, int i) {
  if (i <= 0)
    cythE_error(C, "invalid argument index %d", i);
  return cythE_peek(C, i)->tt_ == CYTH_NONE;
}

CYTH_API cyth_integer cyth_arginteger(cyth_State *C, int i) {
  checkarg(C, CYTH_INTEGER, i);
  return obj2i(cythE_peek(C, i));
}

CYTH_API const char *cyth_argstring(cyth_State *C, int i) {
  checkarg(C, CYTH_STRING, i);
  return s2cstr(obj2s(cythE_peek(C, i)));
}

CYTH_API int cyth_argboolean(cyth_State *C, int i) {
  checkarg(C, CYTH_BOOL, i);
  return obj2b(cythE_peek(C, i));
}

CYTH_API cyth_Cfunction cyth_argcfunction(cyth_State *C, int i) {
  checkarg(C, CYTH_CFUNCTION, i);
  return obj2cf(cythE_peek(C, i));
}

CYTH_API void cyth_call(cyth_State *C, int i, int n, int r) {
  cythF_call(C, i, n, r);
}

CYTH_API void cyth_loadfile(cyth_State *C, const char *filename) {
  cythI_loadfile(C, (char*)filename);
}

CYTH_API void cyth_loadstring(cyth_State *C, const char *chunkname, const char *chunk) {
  cythI_loadstring(C, (char*)chunkname, (char*)chunk);
}

static int generic_writer(cyth_State *C, void *b, size_t size, void *aux) {
  (void)C;
  return fwrite(b, size, 1, (FILE*)aux);
}

CYTH_API void cyth_packprogram(cyth_State *C, int i, const char *outname) {
  checktype(C, CYTH_FUNCTION, i);
  FILE *d = fopen(outname, "wb");
  if (d == NULL) goto defer;
  cythU_unload(C, obj2f(cythE_peek(C, i)), generic_writer, d);
defer:
  if (d != NULL)
    fclose(d);
  else
    cythE_error(C, "could not open file %s: %s", outname, strerror(errno));
}

CYTH_API void cyth_printfunction(cyth_State *C, int i) {
  checktype(C, CYTH_FUNCTION, i);
  cythL_print(obj2f(cythE_peek(C, i)));
}

CYTH_API void cyth_setglobal(cyth_State *C, int i, const char *name) {
  cythV_setglobal(C, cythS_new(C, (char*)name), *cythE_peek(C, i));
}

CYTH_API void cyth_getglobal(cyth_State *C, const char *name) {
  Tvalue res;
  cythV_getglobal(C, cythS_new(C, (char*)name), &res);
  cythA_push(C, res);
}

CYTH_API void cyth_newtable(cyth_State *C) {
  cythA_push(C, t2obj(cythH_new(C)));
}

CYTH_API void cyth_newarray(cyth_State *C) {
  cythA_push(C, a2obj(cythR_new(C)));
}

CYTH_API void cyth_setf(cyth_State *C, int i) {
  Tvalue k;
  cyth_over(C, i);
  cyth_rot(C, -3);
  k = cythA_pop(C);
  cythV_setf(C, k);
}

CYTH_API void cyth_getf(cyth_State *C, int i) {
  Tvalue res, k;
  cyth_over(C, i);
  cyth_rot(C, -2);
  k = cythA_pop(C);
  cythV_getf(C, &res, k);
  cythA_push(C, res);
}

CYTH_API void cyth_setfield(cyth_State *C, int i, const char *name) {
  cyth_pushstring(C, name);
  cyth_rot(C, -2);
  cyth_setf(C, incindex(i, 1));
}

CYTH_API void cyth_getfield(cyth_State *C, int i, const char *name) {
  cyth_pushstring(C, name);
  cyth_getf(C, incindex(i, 1));
}
