#ifndef CSTATE_H
#define CSTATE_H
#include <cobject.h>

#define MINSTACK 20

struct cyth_State {
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
#endif
