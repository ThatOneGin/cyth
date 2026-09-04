#ifndef LIBCYTH_H
#define LIBCYTH_H
#include <stdint.h>
#include <stddef.h>

#define CYTH_INTEGER_MAX INT64_MAX

/* already defined internally */
#ifndef CYTH_INTERNAL
typedef struct cyth_State cyth_State;
typedef int64_t cyth_integer;
typedef int (*cyth_Cfunction)(cyth_State *);
typedef void (*cyth_Destructor)(cyth_State *, void *);
#endif

#ifdef CYTH_INTERNAL
#define CYTH_API
#else
#define CYTH_API extern
#endif

#define cyth_dup(C) cyth_copy(C, -1)
#define cyth_swap(C) cyth_rot(C, -2)

typedef struct {
  void *data;
  size_t size;
} cyth_userdata;

enum CYTH_EQOP {
  CYTH_OPEQ,
  CYTH_OPNE,
  CYTH_OPCOUNT
};

CYTH_API cyth_State *cyth_newstate(void);
CYTH_API void cyth_openstdlib(cyth_State *C);
CYTH_API int cyth_gettop(cyth_State *C);
CYTH_API void cyth_settop(cyth_State *C, int i);
CYTH_API void cyth_destroystate(cyth_State *C);

CYTH_API void cyth_copy(cyth_State *C, int i);
CYTH_API void cyth_rot(cyth_State *C, int i);
CYTH_API void cyth_pop(cyth_State *C);

CYTH_API void cyth_pushnone(cyth_State *C);
CYTH_API void cyth_pushinteger(cyth_State *C, cyth_integer i);
CYTH_API void cyth_pushstring(cyth_State *C, const char *s);
CYTH_API void cyth_pushboolean(cyth_State *C, int b);
CYTH_API void cyth_pushcfunction(cyth_State *C, cyth_Cfunction f);

/* returns true (1) if stack slot i is none value, otherwise returns false (0) */
CYTH_API int cyth_argnone(cyth_State *C, int i);
CYTH_API cyth_integer cyth_arginteger(cyth_State *C, int i);
CYTH_API const char *cyth_argstring(cyth_State *C, int i);
CYTH_API int cyth_argboolean(cyth_State *C, int i);
CYTH_API cyth_Cfunction cyth_argcfunction(cyth_State *C, int i);
CYTH_API cyth_userdata cyth_arguserdata(cyth_State *C, int i);

CYTH_API void cyth_call(cyth_State *C, int i, int n, int r);

CYTH_API void cyth_loadfile(cyth_State *C, const char *filename);
CYTH_API void cyth_loadstring(cyth_State *C, const char *chunkname, const char *chunk);
CYTH_API void cyth_packprogram(cyth_State *C, int i, const char *outname);
CYTH_API void cyth_printfunction(cyth_State *C, int i);

CYTH_API void cyth_setglobal(cyth_State *C, int i, const char *name);
CYTH_API void cyth_getglobal(cyth_State *C, const char *name);

CYTH_API void cyth_newtable(cyth_State *C);
CYTH_API void cyth_newarray(cyth_State *C);

CYTH_API void cyth_setf(cyth_State *C, int i);
CYTH_API void cyth_getf(cyth_State *C, int i);
CYTH_API void cyth_setfield(cyth_State *C, int i, const char *name);
CYTH_API void cyth_getfield(cyth_State *C, int i, const char *name);

CYTH_API void *cyth_newuserdata(cyth_State *C, size_t size);
CYTH_API void cyth_setdestructor(cyth_State *C, int i, cyth_Destructor d);

CYTH_API int cyth_compare(cyth_State *C, int i1, int i2, int op);
#endif