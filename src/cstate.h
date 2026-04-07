#ifndef CSTATE_H
#define CSTATE_H
#include <cobject.h>
#include <cfunc.h>

#define calculate_stack_offset(C, ptr) ((ptr) - ((C)->base.p))
#define apply_stack_offset(C, off) (((C)->base.p) + off)

#define MINSTACK 20
#define MAXCALLS 100

#define GCOS 0
#define GCOT 1
#define GCOF 2
#define GCOU 3

struct gc_object {
  byte tt_;
  union {
    String *s;
    Table *t;
    cyth_Function *f;
    userdata u;
  } v;
  byte mark;
  struct gc_object *next;
};

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

typedef struct {
  stkrel p; /* the pointer to the stack data */
  ptrdiff_t offs; /* field used to save the stack offset when reallocating it */
} stk_t;

typedef struct Call_info {
  struct Call_info *prev;
  stk_t top;
  stk_t func; /* pointer to function in the stack */
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
  Table *gt; /* global table */
  stk_t base; /* first stack element */
  stk_t top; /* first empty slot */
  cmem_t maxoff; /* maximum distance from base to top */
  byte main; /* is it the main state? */
  stringtable cache; /* string cache */
  cyth_jmpbuf *errhandler; /* recover point in case of errors */
  Call_info *ci; /* function call information */
  byte ncalls; /* how many function calls aren't finished */
  byte rebase; /* signal VM to re assign the base variable at the main loop */
};

cyth_State *cythE_openstate(void);
void cythE_newci(cyth_State *C);
cmem_t cythE_gettop(cyth_State *C);
void cythE_closestate(cyth_State *C);
void cythE_error(cyth_State *C, const char *f, ...);
void cythE_inctop(cyth_State *C);
void cythE_dectop(cyth_State *C);
Tvalue *cythE_peek(cyth_State *C, int idx);
void cythE_throw(cyth_State *C, byte errcode, String *errmsg);
byte cythE_runprotected(cyth_State *C, cyth_Pfunction f, void *ud);
#endif
