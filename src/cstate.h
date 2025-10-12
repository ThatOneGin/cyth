#ifndef CSTATE_H
#define CSTATE_H
#include <cobject.h>
#include <cfunc.h>

#define MINSTACK 20
#define MAXCALLS 100

#define GCOS 0
#define GCOT 1
#define GCOF 2

typedef struct gc_object {
  byte tt_;
  union {
    String *s;
    Table *t;
    cyth_Function *f;
  } v;
  byte mark;
  struct gc_object *next;
} gc_object;

typedef struct {
  String **strings;
  cmem_t nstrings;
  cmem_t stringsize;
} stringtable;

typedef struct cyth_jmpbuf {
  String *errmsg;
  byte errcode;
  jmp_buf buf;
  struct cyth_jmpbuf *previous;
} cyth_jmpbuf;

typedef struct Call_info {
  struct Call_info *prev;
  stkrel top;
  stkrel func; /* pointer to function in the stack */
  byte type; /* C or cyth */
  union {
    struct {
      cyth_Function *f; /* function prototype */
      int pc; /* function program counter */
      Table *locvars; /* local variables */
    } cyth;
    struct {
      int nargs;
    } c;
  } u;
} Call_info;

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
  stringtable cache; /* string cache */
  cyth_jmpbuf *errhandler; /* recover point in case of errors */
  Call_info *ci; /* function call information */
  byte ncalls; /* how many function calls aren't finished */
};

cyth_State *cythE_openstate(void);
void cythE_newci(cyth_State *C);
cmem_t cythE_gettop(cyth_State *C);
void cythE_closestate(cyth_State *C);
void cythE_error(cyth_State *C, const char *f, ...);
void cythE_inctop(cyth_State *C);
void cythE_dectop(cyth_State *C);
void cythE_throw(cyth_State *C, byte errcode, String *errmsg);
byte cythE_runprotected(cyth_State *C, cyth_Pfunction f, void *ud);
#endif
