#ifndef CSTRING_H
#define CSTRING_H
#include <cstate.h>

void cythS_init(cyth_State *C);
int cythS_delete(cyth_State *C, String *s);
void cythS_clear(cyth_State *C);
String *cythS_new(cyth_State *C, char *data);
int cythS_streq(String *s1, String *s2);
#endif