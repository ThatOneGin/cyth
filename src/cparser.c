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
#define freturn(ls, line) emitC(ls, (OP_RETURN<<OPCODE_POS), line)

static int emitC(lex_State *ls, Instruction i, int line) {
  cyth_Function *f = ls->fs->f;
  return cythF_emitC(f, i, line);
}

static int emitK(lex_State *ls, Tvalue k) {
  cyth_Function *f = ls->fs->f;
  return cythF_emitK(f, k);
}

static int jmp(lex_State *ls, int i, int line) {
  Instruction inst = 0;
  setopcode(inst, OP_JMP);
  setargz(inst, i);
  return emitC(ls, inst, line);
}

static void jt(lex_State *ls, int line) {
  Instruction inst = 0;
  setopcode(inst, OP_JT);
  emitC(ls, inst, line);
}

static void function(lex_State *ls, int f, int line) {
  Instruction inst = 0;
  setopcode(inst, OP_FUNC);
  setargz(inst, f);
  emitC(ls, inst, line);
}

static void setglobal(lex_State *ls, String *name, int line) {
  Instruction inst = 0;
  setopcode(inst, OP_SETGLB);
  setargz(inst, emitK(ls, s2obj(name)));
  emitC(ls, inst, line);
}

/* Parsing source */

static void ifstat(lex_State *ls);

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
  case '-':
    value(ls, res);
    if (cyth_tt(res) != CYTH_INTEGER)
      cythL_syntaxerror(ls, "- Unary operator requires a number.");
    else
      obj2i(res) *= -1;
    break;
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
  int line = saveline(ls);
  Instruction i = 0;
  Token opcode = next(ls);
  switch (opcode.type) {
  case TK_CONST: {
    setopcode(i, OP_PUSH);
    int argz;
    Tvalue k;
    value(ls, &k);
    argz = emitK(ls, k);
    setargz(i, argz);
  } break;
  case TK_RETURN: {
    setopcode(i, OP_RETURN);
  } break;
  case TK_ADD: {
    setopcode(i, OP_ADD);
  } break;
  case TK_SETVAR: {
    String *name = expect(ls, TK_NAME, "Expected identifier.").value.s;
    setopcode(i, OP_SETVAR);
    setargz(i, emitK(ls, s2obj(name)));
  } break;
  case TK_GETVAR: {
    String *name = expect(ls, TK_NAME, "Expected identifier.").value.s;
    setopcode(i, OP_GETVAR);
    setargz(i, emitK(ls, s2obj(name)));
  } break;
  case TK_EQ: {
    setopcode(i, OP_EQ);
  } break;
  case TK_NEQ: {
    setopcode(i, OP_NEQ);
  } break;
  default:
    error_unknown(ls, "instruction name");
    break;
  }
  emitC(ls, i, line);
  expect(ls, ';', "';'");
}

static void instblock(lex_State *ls) {
  switch (ls->t.type) {
  case '(': /* a block */
    next(ls);
    ifstat(ls);
    break;
  default:
    instruction(ls);
    break;
  }
}

static void blockbody(lex_State *ls, int stop) {
  do {
    instblock(ls);
  } while (!match(ls, stop) && !match(ls, TK_EOF));
}

/* '(' if '('instlist')' instlist ')' */
static void ifstat(lex_State *ls) {
  expect(ls, TK_IF, "Expected 'if'.");
  expect(ls, '(', "Expected condition");
  blockbody(ls, ')');
  expect(ls, ')', "Expected end of condition");
  jt(ls, saveline(ls));
  int to_patch = jmp(ls, 0, saveline(ls));
  blockbody(ls, ')');
  expect(ls, ')', "Expected ')");
  setargz(ls->fs->f->code[to_patch], (ls->fs->f->ncode - to_patch - 1));
}

/* func '(' ')' instlist ')' */
static void func(lex_State *ls) {
  openfunc(ls);
  cyth_Function *f = ls->fs->f;
  String *name;
  int start_line = saveline(ls);
  expect(ls, '(', "'('");
  expect(ls, TK_FUNC, "'func' keyword.");
  name = expect(ls, TK_NAME, "identifier").value.s;
  {
    expect(ls, '(', "'('");
    /* TODO: parameters */
    expect(ls, ')', "')'");
  }
  blockbody(ls, ')');
  int end_line = saveline(ls);
  expect(ls, ')', "')'");
  freturn(ls, end_line); /* last instruction (return) */
  closefunc(ls);
  int fidx = cythF_emitF(ls->fs->f, f);
  function(ls, fidx, end_line);
  setglobal(ls, name, start_line);
}

static void mainfunc(lex_State *ls) {
  openfunc(ls);
  cyth_Function *f = ls->fs->f;
  while (ls->t.type == '(') {
    func(ls);
  }
  expect(ls, TK_EOF, "End-of-file");
  freturn(ls, saveline(ls));
  closefunc(ls);
  cythA_push(ls->C, f2obj(f));
}

/* parse the main function of a chunk */
cyth_Function *cythP_parse(cyth_State *C, Stream *input, char *chunkname) {
  lex_State ls = cythL_new(C, chunkname, input);
  cythL_next(&ls);
  mainfunc(&ls);
  return obj2f(&C->top[-1]); /* function is at the top */
}

#undef saveline
#undef freturn