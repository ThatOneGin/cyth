/*
** order of reserved words:
** TK_FUNC, TK_CONST, TK_RETURN, TK_ADD, TK_SUB, TK_DIV, TK_MUL
** TK_SETVAR, TK_GETVAR, TK_EQ, TK_NEQ,
** TK_JF, TK_CALL, TK_JT, TK_DUP, TK_SWAP, TK_TRUE, TK_FALSE, TK_JMP, TK_POP
** last reserved kind must be before TK_NAME
*/
#ifndef CLEX_CYTH_H
#define CLEX_CYTH_H
#include <clex.h>

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
  TK_FUNC = FIRSTRESERVED, TK_CONST, TK_RETURN, TK_ADD, TK_SUB, TK_DIV, TK_MUL,
  TK_SETVAR, TK_GETVAR,
  TK_EQ, TK_NEQ, TK_JF, TK_CALL, TK_JT, TK_DUP, TK_SWAP, TK_TRUE, TK_FALSE, TK_JMP,
  TK_POP,
/* end reserved */
  TK_NAME, TK_INT, TK_STR,

  TK_ARROW /* -> */
};

void lexcinit(lex_State *ls);
void lexc(lex_State *ls);
#endif
