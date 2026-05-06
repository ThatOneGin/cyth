#include <clex_cx.h>
#include <clex.h>

#define next(ls) ((ls)->current = cythI_getc((ls)->input))
#define save(c) (buf[pos++] = (c))
#define expect(ls, c, err) \
  (((ls)->current == (c)) ? ((void)next(ls)) : \
    cythL_syntaxerror(ls, err))

static char *reserved[] = {
  "func", "const", "return",
  "true", "false", "do", "end", "if"
};

static void read_string(lex_State *ls) {
  expect(ls, '"', "expected '\"'");
  while (ls->current != '"' &&
         ls->current != '\n' &&
         ls->current != EOS) {
    switch (ls->current) {
    case '\\':
      next(ls);
      switch (ls->current) {
      case 'n': cythO_buffer_appendchar(ls->C, &ls->buf, '\n'); break;
      case 'r': cythO_buffer_appendchar(ls->C, &ls->buf, '\r'); break;
      case 'f': cythO_buffer_appendchar(ls->C, &ls->buf, '\f'); break;
      case 't': cythO_buffer_appendchar(ls->C, &ls->buf, '\t'); break;
      case 'b': cythO_buffer_appendchar(ls->C, &ls->buf, '\b'); break;
      case 'a': cythO_buffer_appendchar(ls->C, &ls->buf, '\a'); break;
      case 'v': cythO_buffer_appendchar(ls->C, &ls->buf, '\v'); break;
      case '\\': cythO_buffer_appendchar(ls->C, &ls->buf, '\\'); break;
      case '\"': cythO_buffer_appendchar(ls->C, &ls->buf, '\"'); break;
      default:
        cythL_syntaxerror(ls, s2cstr(
          cythS_sprintf(ls->C, "Unknown escape sequence '\\%c'", ls->current)));
        break;
      }
      next(ls);
      break;
    default:
      cythO_buffer_appendchar(ls->C, &ls->buf, ls->current);
      next(ls);
      break;
    }
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

void lexxinit(lex_State *ls) {
  String *s;
  for (int i = 0; i < NUMRESERVED; i++) {
    s = cythL_createstring(ls, reserved[i]);
    s->aux = (sbyte)i;
  }
}

void lexx(lex_State *ls) {
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
    lexx(ls);
    return;
  } else if (ls->current == '#') {
    skip_comment(ls);
    lexx(ls);
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
    if (s->aux >= 0) { /* it's a reserved word */
      ls->t.type = FIRSTRESERVED + s->aux;
    } else { /* identifier */
      ls->t.type = TK_NAME;
      cythL_anchorstring(ls, s);
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
    case '-':
      next(ls);
      if (ls->current == '>') {
        ls->t.type = TK_ARROW;
        ls->t.value.i = TK_ARROW;
      } else
        ls->t.type = ls->t.value.i = '-';
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
