/*
** order of reserved words:
** TK_FUNC, TK_CONST, TK_RETURN, TK_ADD
** last reserved kind must be before TK_NAME
*/
#ifndef CLEX_H
#define CLEX_H
#include <cstring.h>
#include <cobject.h>
#include <cio.h>

/*
** Terminal symbols that are also single chars
** are represented by their numeric values
** and other tokens such as reserved keywords
**  start at UCHAR_MAX + 1.
*/
#define FIRSTRESERVED (UCHAR_MAX + 1)
#define NUMRESERVED (TK_NAME - FIRSTRESERVED)
#define maxidentsize 256

enum tkreserved {
  TK_EOF,
  TK_FUNC = FIRSTRESERVED, TK_CONST, TK_RETURN, TK_ADD,
  TK_NAME, TK_INT, TK_STR
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
} lex_State;

lex_State *cythL_new(cyth_State *C, char *name, Stream *input);
String *cythL_createstring(lex_State *ls, char *s);
void cythL_syntaxerror(lex_State *ls, const char *s);
void cythL_next(lex_State *ls);
void cythL_free(lex_State *ls);
#endif
