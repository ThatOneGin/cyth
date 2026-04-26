/*
** order of reserved words:
** TK_FUNC, TK_CONST, TK_RETURN,
** TK_TRUE, TK_FALSE
** last reserved kind must be before TK_NAME
*/
#ifndef CLEX_CX_H
#define CLEX_CX_H
#include <clex.h>
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
/* begin reserved */
  TK_FUNC = FIRSTRESERVED, TK_CONST, TK_RETURN,
  TK_TRUE, TK_FALSE,
/* end reserved */
  TK_NAME, TK_INT, TK_STR,

  TK_ARROW /* -> */
};

void lexxinit(lex_State *ls);
void lexx(lex_State *ls);
#endif
