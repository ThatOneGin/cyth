#include <clex.h>
#include <caux.h>
#include <cmem.h>
#include <ctype.h>
#include <stdarg.h>

/* cx */
extern void lexxinit(lex_State *ls);
extern void lexx(lex_State *ls);
/* cyth */
extern void lexcinit(lex_State *ls);
extern void lexc(lex_State *ls);

/*
** Anchor a string to the lexer table
** so it won't get collected by the GC
*/
void cythL_anchorstring(lex_State *ls, String *s) {
  cythR_push(ls->C, ls->tab, ls->tab->narray, s2obj(s));
}

lex_State cythL_new(cyth_State *C, int mode, char *name, Stream *input) {
  lex_State ls = {.mode = mode};
  ls.C = C;
  ls.tab = cythR_new(C);
  /* put the table where the GC can see */
  cythA_push(C, a2obj(ls.tab));
  cythO_buffer_new(&ls.buf);
  ls.line = 1;
  ls.fs = NULL;
  ls.sourcename = cythL_createstring(&ls, name);
  ls.input = input;
  ls.current = cythI_getc(ls.input);
  if (mode == LEXMCYTH) lexcinit(&ls);
  else if (mode == LEXMCX) lexxinit(&ls);
  else cyth_assert(0);
  return ls;
}

String *cythL_createstring(lex_State *ls, char *s) {
  String *sdesc = cythS_new(ls->C, s);
  cythL_anchorstring(ls, sdesc);
  return sdesc;
}

void cythL_syntaxerror(lex_State *ls, const char *f, ...) {
  char *modename = ls->mode == LEXMCYTH ? "cyth" : "cx";
  String *errmsg = NULL;
  va_list ap;
  va_start(ap, f);
  errmsg = cythS_vsprintf(ls->C, f, ap);
  va_end(ap);
  errmsg =
    cythS_sprintf(ls->C, "%s: %s: syntax error near line %d: %s\n",
    modename, s2cstr(ls->sourcename), ls->line, s2cstr(errmsg));
  cythE_throw(ls->C, CYTH_SYNTAXERR, errmsg);
}

/* get next token stored in ls->t */
void cythL_next(lex_State *ls) {
  if (ls->mode == LEXMCYTH) lexc(ls);
  else if (ls->mode == LEXMCX) lexx(ls);
  else cythL_syntaxerror(ls, "invalid mode");
}

#undef next
#undef save
#undef expect
