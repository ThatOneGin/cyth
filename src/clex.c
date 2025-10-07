#include <clex.h>
#include <caux.h>
#include <cmem.h>
#include <ctype.h>

#define next(ls) ((ls)->current = cythI_getc((ls)->input))
#define save(c) (buf[pos++] = (c))

/* don't change order. */
static char *reserved[] = {
  "func", "const", "return", "add"
};

/*
** Anchor a string to the lexer table
** so it won't get collected by the GC
*/
static void anchorstring(lex_State *ls, String *s) {
  cythH_append(ls->C, ls->tab, i2obj(ls->tab->len), s2obj(s));
}

lex_State *cythL_new(cyth_State *C, char *name, Stream *input) {
  lex_State *ls = cythM_malloc(C, sizeof(lex_State));
  ls->C = C;
  ls->tab = cythH_new(C);
  /* put the table where the GC can see */
  cythA_push(C, t2obj(ls->tab));
  ls->line = 1;
  ls->fs = NULL;
  ls->sourcename = cythL_createstring(ls, name);
  ls->input = input;
  ls->current = cythI_getc(ls->input);
  anchorstring(ls, ls->sourcename);
  String *s;
  for (int i = 0; i < NUMRESERVED; i++) {
    s = cythL_createstring(ls, reserved[i]);
    s->aux = (byte)i;
    anchorstring(ls, s);
  }
  return ls;
}

String *cythL_createstring(lex_State *ls, char *s) {
  String *sdesc = cythS_new(ls->C, s);
  return sdesc;
}

void cythL_syntaxerror(lex_State *ls, const char *s) {
  cythE_error(ls->C, "%s: Syntax error ne ar line %d: %s",
    s2cstr(ls->sourcename), ls->line, s);
}

static inline int c_isident(int c) {
  return isalpha(c) || c == '_' || c == '-';
}

static inline int c_isnum(int c) {
  return isdigit(c);
}

static inline int c_isspace(int c) {
  return isspace(c);
}

/* get next token stored in ls->t */
void cythL_next(lex_State *ls) {
  char buf[maxidentsize];
  byte pos = 0;
  if (ls->current == EOS) {
    ls->t.type = TK_EOF;
    return;
  } else if (c_isspace(ls->current)) {
    while (c_isspace(ls->current)) {
      if (ls->current == '\n' || ls->current == '\r')
        ls->line++;
      next(ls);
    }
  }
  if (c_isident(ls->current)) {
    while (c_isident(ls->current) && pos < (maxidentsize-1)) {
      save(ls->current);
      next(ls);
    }
    if (c_isident(ls->current)) { /* TODO: break this limit */
      cythL_syntaxerror(ls, "Maximum identifier length reached.");
    }
    save(0);
    String *s = cythS_new(ls->C, buf);
    if (s->aux < 255) { /* it's a reserved word */
      ls->t.type = FIRSTRESERVED + s->aux;
    } else { /* identifier */
      ls->t.type = TK_NAME;
      anchorstring(ls, s);
    }
    ls->t.value.s = s;
  } else if (c_isnum(ls->current)) { /* only integers for now */
    cyth_integer intbuf = 0;
    while (c_isnum(ls->current)) {
      intbuf = (intbuf * 10) + (ls->current - '0');
      next(ls);
    }
    ls->t.type = TK_INT;
    ls->t.value.i = intbuf;
  } else {
    if (!iscntrl(ls->current)) {
      ls->t.value.i = ls->t.type = ls->current;
      next(ls);
    } else {
      String *msg = cythS_sprintf(ls->C,
        "Unknown char <%d>.", ls->current);
      cythL_syntaxerror(ls, s2cstr(msg));
    }
  }
}

void cythL_free(lex_State *ls) {
  ls->current = 0;
  ls->line = 0;
  cythM_free(ls->C, ls, sizeof(lex_State));
}

#undef next
#undef save