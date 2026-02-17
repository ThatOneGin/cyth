#include <clex.h>
#include <caux.h>
#include <cmem.h>
#include <ctype.h>

#define next(ls) ((ls)->current = cythI_getc((ls)->input))
#define save(c) (buf[pos++] = (c))
#define expect(ls, c, err) \
  (((ls)->current == (c)) ? ((void)next(ls)) : \
    cythL_syntaxerror(ls, err))

/* don't change order. */
static char *reserved[] = {
  "func", "const", "return", "add", "setvar",
  "getvar", "eq", "neq", "if", "call", "while", "dup", "swap"
};

/*
** Anchor a string to the lexer table
** so it won't get collected by the GC
*/
static void anchorstring(lex_State *ls, String *s) {
  cythH_append(ls->C, ls->tab, i2obj(ls->tab->len), s2obj(s));
}

lex_State cythL_new(cyth_State *C, char *name, Stream *input) {
  lex_State ls = {0};
  ls.C = C;
  ls.tab = cythH_new(C);
  /* put the table where the GC can see */
  cythA_push(C, t2obj(ls.tab));
  cythO_buffer_new(&ls.buf);
  ls.line = 1;
  ls.fs = NULL;
  ls.sourcename = cythL_createstring(&ls, name);
  ls.input = input;
  ls.current = cythI_getc(ls.input);
  anchorstring(&ls, ls.sourcename);
  String *s;
  for (int i = 0; i < NUMRESERVED; i++) {
    s = cythL_createstring(&ls, reserved[i]);
    s->aux = (byte)i;
    anchorstring(&ls, s);
  }
  return ls;
}

String *cythL_createstring(lex_State *ls, char *s) {
  String *sdesc = cythS_new(ls->C, s);
  return sdesc;
}

void cythL_syntaxerror(lex_State *ls, const char *s) {
  cythE_error(ls->C, "%s: Syntax error near line %d: %s",
    s2cstr(ls->sourcename), ls->line, s);
}

static inline int c_isident(int c) {
  return isalpha(c) || c == '_';
}

static inline int c_isnum(int c) {
  return isdigit(c);
}

static inline int c_isspace(int c) {
  return isspace(c);
}

static void read_string(lex_State *ls) {
  expect(ls, '"', "expected '\"'");
  while (ls->current != '"' &&
         ls->current != '\n' &&
         ls->current != EOS) {
    cythO_buffer_appendchar(ls->C, &ls->buf, ls->current);
    next(ls);
  }
  if (ls->current != '"') {
    cythL_syntaxerror(ls, "Unfinished string.");
  } else
    next(ls);
  String *tks;
  if (ls->buf.n == 0) {
    tks = cythS_new(ls->C, "");
  } else {
    cythO_buffer_appendchar(ls->C, &ls->buf, 0);
    tks = cythS_new(ls->C, ls->buf.data);
  }
  ls->t.type = TK_STR;
  ls->t.value.s = tks;
  cythO_buffer_rewind(ls->C, &ls->buf);
}

static void skip_comment(lex_State *ls) {
  expect(ls, '#', "Expected '#'");
  while (ls->current != '\n' &&
         ls->current != '\r' &&
         ls->current != EOS) {
    next(ls);
  }
  if (ls->current != EOS) {
    next(ls);
    ls->line++;
  }
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
    cythL_next(ls);
    return;
  } else if (ls->current == '#') {
    skip_comment(ls);
    cythL_next(ls);
    return;
  }
  if (c_isident(ls->current)) {
    do {
      save(ls->current);
      next(ls);
    } while ((ls->current == '-' || c_isident(ls->current)) && pos < (maxidentsize-1));
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
    switch (ls->current) {
    case '"':
      read_string(ls);
      break;
    default:
      if (!iscntrl(ls->current)) {
        ls->t.value.i = ls->t.type = ls->current;
        next(ls);
      } else {
        String *msg = cythS_sprintf(ls->C,
        "Unknown char <%d>.", ls->current);
        cythL_syntaxerror(ls, s2cstr(msg));
      }
      break;
    }
  }
}

#undef next
#undef save
#undef expect
