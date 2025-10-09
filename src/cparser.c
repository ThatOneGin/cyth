#include <cparser.h>
#include <cmem.h>
#include <caux.h>

/*
** Ok so, normally we should construct an AST and then compile it (2 steps)
** But for a language like this (unless we have more complex semantic analisys)
** we don't actually need to do that, just get some information and compile it.
*/

/* Code generation */

#define saveline(ls) ((ls)->line)

static int emitC(lex_State *ls, Instruction i, int line) {
  cyth_Function *f = ls->fs->f;
  return cythF_emitC(f, i, line);
}

static int emitK(lex_State *ls, Tvalue k) {
  cyth_Function *f = ls->fs->f;
  return cythF_emitK(f, k);
}

/* Parsing source */

static void error_unknown(lex_State *ls, const char *what) {
  char buf[BUFFERSIZE];
  snprintf(buf, BUFFERSIZE, "Unknown %s.", what);
  cythL_syntaxerror(ls, buf);
}

static void error_expect(lex_State *ls, const char *what) {
  char buf[BUFFERSIZE];
  snprintf(buf, BUFFERSIZE, "Unexpected token (expected %s).", what);
  cythL_syntaxerror(ls, buf);
}

static inline int match(lex_State *ls, int type) {
  return ls->t.type == type;
}

static Token next(lex_State *ls) {
  Token t = ls->t;
  cythL_next(ls);
  return t;
}

static Token expect(lex_State *ls, int type, char *what) {
  Token t = ls->t;
  if (ls->t.type != type) error_expect(ls, what);
  else cythL_next(ls);
  return t;
}

static void openfunc(lex_State *ls) {
  func_State *fs = cythM_malloc(ls->C, sizeof(func_State));
  fs->ls = ls;
  fs->f = cythF_newfunc(ls->C);
  fs->prev = ls->fs;
  ls->fs = fs;
}

static void closefunc(lex_State *ls) {
  func_State *fs = ls->fs->prev;
  cythM_free(ls->C, ls->fs, sizeof(func_State));
  ls->fs = fs;
}

/* parse a single value */
static void value(lex_State *ls, Tvalue *res) {
  Token t = next(ls);
  switch (t.type) {
  case TK_INT:
    *res = i2obj(t.value.i);
    break;
  case TK_STR:
    *res = s2obj(t.value.s);
    break;
  default:
    cythL_syntaxerror(ls, "Invalid value specifier.");
    break;
  }
}

/* parse an instruction */
static void instruction(lex_State *ls) {
  int line;
  Instruction i = 0;
  Token opcode = next(ls);
  switch (opcode.type) {
  case TK_CONST: {
    setopcode(i, OP_PUSH);
    line = saveline(ls);
    int argz;
    Tvalue k;
    value(ls, &k);
    argz = emitK(ls, k);
    setargz(i, argz);
  } break;
  case TK_RETURN: {
    line = saveline(ls);
    setopcode(i, OP_RETURN);
  } break;
  case TK_ADD: {
    line = saveline(ls);
    setopcode(i, OP_ADD);
  } break;
  default:
    error_unknown(ls, "instruction name");
    break;
  }
  emitC(ls, i, line);
  expect(ls, ';', "';'");
}

static void instlist(lex_State *ls, int stop) {
  do {
    instruction(ls);
  } while (!match(ls, stop));
}

static void func(lex_State *ls) {
  String *name;
  expect(ls, '(', "'('");
  expect(ls, TK_FUNC, "'func' keyword.");
  name = expect(ls, TK_NAME, "identifier").value.s;
  {
    expect(ls, '(', "'('");
    /* TODO: parameters */
    expect(ls, ')', "')'");
  }
  emitK(ls, s2obj(name));
  instlist(ls, ')');
  expect(ls, ')', "')'");
}

/* parse the main function of a chunk */
void cythP_parse(cyth_State *C, Stream *input, char *chunkname) {
  lex_State *ls = cythL_new(C, chunkname, input);
  cythL_next(ls);
  openfunc(ls);
  func(ls);
  cythA_push(C, f2obj(ls->fs->f));
  closefunc(ls);
  cythL_free(ls);
}
