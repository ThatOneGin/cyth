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
  i = ((i << 1) | 0);
  Instruction inst = 0;
  setopcode(inst, OP_JMP);
  setargz(inst, i);
  return emitC(ls, inst, line);
}

static int jmpback(lex_State *ls, int i, int line) {
  int offset = (i << 1) | 1;
  Instruction inst = 0;
  setopcode(inst, OP_JMP);
  setargz(inst, offset);
  return emitC(ls, inst, line);
}

static void patchjmp(lex_State *ls, unsigned int idx,
                     int i, int back) {
  func_State *fs = ls->fs;
  if (idx >= fs->f->ncode) 
    cythL_syntaxerror(ls, "Invalid jmp patch.\n");
  setargz(fs->f->code[idx], ((i << 1) | (back?1:0)));
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

static void setvar(lex_State *ls, Vardsc v) {
  DataBlk *blk = (DataBlk*)ls->pdata;
  if (blk->nvars >= blk->varsize)
    cythM_vecgrow(ls->C, blk->vars, blk->varsize, Vardsc);
  for (cmem_t i = 0; i < blk->nvars; i++) {
    if (blk->vars[i].name == v.name) {
      cythE_error(ls->C, "Trying to redefine variable '%*s'",
        (unsigned int)v.name->len, v.name->data);
    }
  }
  blk->vars[blk->nvars++] = v;
}

static void getvar(lex_State *ls, String *name, Vardsc *v) {
  Vardsc dummy = {0};
  if (v == NULL) /* result is unused */
    v = &dummy;
  DataBlk *blk = (DataBlk*)ls->pdata;
  for (cmem_t i = 0; i < blk->nvars; i++) {
    if (blk->vars[i].name == name) {
      *v = blk->vars[i];
      return;
    }
  }
  cythE_error(ls->C, "Unknown variable '%*s'",
    (unsigned int)name->len, name->data);
}

/* enter new scope */
static void enter(lex_State *ls, cmem_t *nvars) {
  DataBlk *blk = (DataBlk*)ls->pdata;
  *nvars = blk->nvars;
}

/* leave scope (erase scope's local variables) */
static void leave(lex_State *ls, cmem_t nvars) {
  DataBlk *blk = (DataBlk*)ls->pdata;
  blk->nvars = nvars;
}

/* Parsing source */

static void ifstat(lex_State *ls);
static void whilestat(lex_State *ls);

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
  case TK_RETURN: setopcode(i, OP_RETURN); break;
  case TK_ADD: setopcode(i, OP_ADD); break;
  case TK_SETVAR: {
    Vardsc var = {0};
    String *name = expect(ls, TK_NAME, "Expected identifier.").value.s;
    var.k = VKLOC;
    var.name = name;
    var.i = 0;
    setvar(ls, var);
    setopcode(i, OP_SETVAR);
    setargz(i, emitK(ls, s2obj(name)));
  } break;
  case TK_GETVAR: {
    Vardsc v;
    String *name = expect(ls, TK_NAME, "Expected identifier.").value.s;
    getvar(ls, name, &v);
    if (v.k != VKFUN) {
      setopcode(i, OP_GETVAR);
      setargz(i, emitK(ls, s2obj(name)));
    } else { /* a function */
      setopcode(i, OP_GETGLB);
      setargz(i, emitK(ls, s2obj(name)));
    }
  } break;
  case TK_EQ: setopcode(i, OP_EQ); break;
  case TK_NEQ: setopcode(i, OP_NEQ); break;
  case TK_CALL: {
    int nargs = expect(ls, TK_INT,
      "Expected number of arguments"
      " for call instruction.").value.i;
    setopcode(i, OP_CALL);
    setargz(i, nargs);
  } break;
  case TK_DUP: setopcode(i, OP_DUP); break;
  case TK_SWAP: setopcode(i, OP_SWAP); break;
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
    switch (ls->t.type) {
    case TK_IF:
      ifstat(ls);
      break;
    case TK_WHILE:
      whilestat(ls);
      break;
    default:
      cythL_syntaxerror(ls, "Unknown block name");
      break;
    }
    break;
  default:
    instruction(ls);
    break;
  }
}

static void blockbody(lex_State *ls, int stop) {
  cmem_t nvars = 0;
  enter(ls, &nvars);
  while (!match(ls, stop) && !match(ls, TK_EOF)) {
    instblock(ls);
  }
  leave(ls, nvars);
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
  expect(ls, ')', "Expected ')'");
  setargz(ls->fs->f->code[to_patch], (ls->fs->f->ncode - to_patch - 1));
}

static void whilestat(lex_State *ls) {
  func_State *fs = ls->fs;
  expect(ls, TK_WHILE, "Expected 'while'.");
  expect(ls, '(', "Expected condition");
  int init = fs->f->ncode;
  blockbody(ls, ')');
  expect(ls, ')', "Expected end of condition");
  jt(ls, saveline(ls));
  int topatch = jmp(ls, 0, saveline(ls));
  blockbody(ls, ')');
  expect(ls, ')', "Expected ')'");
  jmpback(ls, fs->f->ncode - init + 1, saveline(ls));
  patchjmp(ls, topatch, fs->f->ncode - topatch, 0);
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
  Vardsc var = {0};
  var.k = VKFUN;
  var.i = fidx;
  var.name = name;
  setvar(ls, var);
}

static void mainfunc(lex_State *ls) {
  openfunc(ls);
  cyth_Function *f = ls->fs->f;
  while (ls->t.type == '(') {
    func(ls);
  }
  expect(ls, TK_EOF, "End-of-file");
  Vardsc v;
  getvar(ls, cythS_new(ls->C, "main"), &v);
  if (v.k != VKFUN)
    cythE_error(ls->C, "Expected symbol 'main' to be a function.");
  else {
    Instruction i = 0;
    setopcode(i, OP_FUNC);
    setargz(i, v.i);
    emitC(ls, i, saveline(ls));
    setopcode(i, OP_CALL);
    setargz(i, 0);
    emitC(ls, i, saveline(ls));
  }
  freturn(ls, saveline(ls));
  closefunc(ls);
  cythA_push(ls->C, f2obj(f));
}

static int pmainfunc(cyth_State *C, void *aux) {
  (void)C;
  mainfunc((lex_State*)aux);
  cythA_pushint(C, 0);
  return 0;
}

static void close_funcs(lex_State *ls) {
  func_State *fs = ls->fs;
  func_State *prev = NULL;
  while (fs != NULL) {
    prev = fs->prev;
    cythM_free(ls->C, fs, sizeof(fs));
    fs = prev;
  }
}

/* parse the main function of a chunk */
cyth_Function *cythP_parse(cyth_State *C, Stream *input, char *chunkname) {
  cyth_Function *f = NULL;
  DataBlk blk = {0};
  lex_State ls = cythL_new(C, chunkname, input);
  ls.pdata = (void*)&blk;
  cythL_next(&ls);
  cythE_runprotected(C, pmainfunc, &ls);
  if (!cythA_popint(C)) {
    /* if it doesn't fails, f is the top of the stack (the main function) */
    f = obj2f(&C->top[-1]);
  } else close_funcs(&ls);
  cythM_vecfree(C, blk.vars, blk.varsize, Vardsc);
  return f; /* function is either null or the top */
}

#undef saveline
#undef freturn