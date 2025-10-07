#ifndef CPARSER_H
#define CPARSER_H
#include <clex.h>
#include <cobject.h>

/*
** Auxiliary state to help build
** functions.
*/
struct func_State {
  cyth_Function *f; /* function prototype */
  struct func_State *prev; /* chain of functions */
  lex_State *ls;
};

void cythP_parse(cyth_State *C, Stream *input, char *chunkname);
#endif