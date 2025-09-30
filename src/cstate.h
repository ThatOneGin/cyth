#ifndef CSTATE_H
#define CSTATE_H
#include <cobject.h>

#define MINSTACK 20

#define GCOS 0

typedef struct gc_object {
  byte tt_;
  union {
    String s;
  } v;
  byte mark;
  struct gc_object *next;
} gc_object;

struct global_State {
  gc_object *list; /* objects that live on the stack */
  gc_object *uncollectables; /* objects that live until the main state dies */
  cmem_t count; /* memory in use */
  cmem_t total; /* total memory allocated */
};

struct cyth_State {
  global_State *G;
  stkrel base; /* first stack element */
  stkrel top; /* first empty slot */
  cmem_t maxoff; /* maximum distance from base to top */
  byte main; /* is it the main state? */
};

cyth_State *cythE_openstate(void);
cmem_t cythE_gettop(cyth_State *C);
void cythE_closestate(cyth_State *C);
void cythE_error(cyth_State *C, const char *f, ...);
void cythE_inctop(cyth_State *C);
void cythE_dectop(cyth_State *C);
#endif
