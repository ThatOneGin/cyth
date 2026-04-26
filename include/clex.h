#ifndef CLEX_H
#define CLEX_H
#include <cstring.h>
#include <cobject.h>
#include <cio.h>
#include <ctype.h>

enum LEXMODE {
  LEXMCYTH,
  LEXMCX
};

typedef union {
  cyth_integer i;
  String *s;
} token_Value;

typedef struct {
  int type;
  token_Value value;
} Token;

typedef struct func_State func_State;

typedef struct {
  int line;
  int current; /* current char */
  Token t; /* current token */
  Table *tab; /* anchor for strings */
  func_State *fs; /* used by the parser */
  cyth_State *C;
  Stream *input;
  String *sourcename;
  SBuffer buf; /* buffer for string building */
  void *pdata; /* used by the paser (set later) */
  const int mode; /* LEXMODE */
} lex_State;

static inline int c_isident(int c) {return isalpha(c) || c == '_';}
static inline int c_isnum(int c) {return isdigit(c);}
static inline int c_isspace(int c) {return isspace(c);}

lex_State cythL_new(cyth_State *C, int mode, char *name, Stream *input);
void cythL_anchorstring(lex_State *ls, String *s);
String *cythL_createstring(lex_State *ls, char *s);
void cythL_syntaxerror(lex_State *ls, const char *s);
void cythL_next(lex_State *ls);
#endif
