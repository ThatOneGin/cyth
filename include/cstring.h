#ifndef CSTRING_H
#define CSTRING_H
#include <cstate.h>
#include <stdarg.h>

#define BUFFERSIZE 1024

#define s2cstr(s) ((s)->data)

void cythS_init(cyth_State *C);
void cythS_clear(cyth_State *C);
String *cythS_new(cyth_State *C, char *data);
int cythS_streq(String *s1, String *s2);
String *cythS_vsprintf(cyth_State *C, const char *f, va_list ap);
String *cythS_sprintf(cyth_State *C, const char *f, ...);
String *cythS_newstrobj(cyth_State *C, cmem_t len);
void cythS_finishstrobj(cyth_State *C, String **s);
#endif