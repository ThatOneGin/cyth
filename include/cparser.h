#ifndef CPARSER_H
#define CPARSER_H
#include <clex.h>
#include <cobject.h>
#include <cio.h>

#define VKLOC 0
#define VKFUN 1
#define VKGLB 2

typedef struct {
  byte k; /* VKLOC or VKFUN */
  String *name;
  int i; /* info about the variable (could be an index, a type or simply nothing) */
} Vardsc;

typedef struct {
  String *name;
  int pc;
  int patch;
} Labeldsc;

typedef struct {
  Vardsc *vars;
  cmem_t n;
  cmem_t s;
} varlist;

typedef struct {
  Labeldsc *labels;
  cmem_t n;
  cmem_t s;
} lablist;

typedef struct {
  varlist vars;
  lablist labels;
} DataBlk;

/*
** Auxiliary state to help build
** functions.
*/
struct func_State {
  cyth_Function *f; /* function prototype */
  struct func_State *prev; /* chain of functions */
  lex_State *ls;
};

cyth_Function *cythP_parse_cx(cyth_State *C, Stream *input, char *chunkname);
cyth_Function *cythP_parse_cyth(cyth_State *C, Stream *input, char *chunkname);
#endif