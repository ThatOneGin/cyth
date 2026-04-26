#include <cparser.h>
#include <clex_cyth.h>
#include <cio.h>
#include <cmem.h>
#include <caux.h>

#define NOPATCH 0
#define NEEDPATCH 1

/*
** Ok so, normally we should construct an AST and then compile it (2 steps)
** But for a language like this (unless we have more complex semantic analysis)
** we don't actually need to do that, just get some information and compile it.
*/

/* carries current scope information */
typedef struct {
  int nlabels;
  int nvars;
} Symtab;

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
  if (blk->vars.n >= blk->vars.s)
    cythM_vecgrow(ls->C, blk->vars.vars, blk->vars.s, Vardsc);
  for (cmem_t i = 0; i < blk->vars.n; i++) {
    if (blk->vars.vars[i].name == v.name) {
      cythE_error(ls->C, "Trying to redefine variable '%*s'",
        (unsigned int)v.name->len, v.name->data);
    }
  }
  blk->vars.vars[blk->vars.n++] = v;
}

static void getvar(lex_State *ls, String *name, Vardsc *v) {
  Vardsc dummy = {0};
  if (v == NULL) /* result is unused */
    v = &dummy;
  DataBlk *blk = (DataBlk*)ls->pdata;
  for (cmem_t i = 0; i < blk->vars.n; i++) {
    if (blk->vars.vars[i].name == name) {
      *v = blk->vars.vars[i];
      return;
    }
  }
  v->k = VKGLB;
  v->name = name;
}

/* enter new scope */
static void enter(lex_State *ls, Symtab *sym) {
  DataBlk *blk = (DataBlk*)ls->pdata;
  sym->nvars = blk->vars.n;
  sym->nlabels = blk->labels.n;
}

/* leave scope (erase scope's local variables) */
static void leave(lex_State *ls, Symtab sym) {
  DataBlk *blk = (DataBlk*)ls->pdata;
  blk->vars.n = sym.nvars;
  blk->labels.n = sym.nlabels;
}

static inline int relpc(lex_State *ls) {
  return ls->fs->f->ncode;
}

static void patchlab(lex_State *ls, int where, Labeldsc l) {
  func_State *fs = ls->fs;
  Instruction *i = &fs->f->code[where];
  setargz(*i, cythC_imm_new(l.pc - where - 1));
}

static void setlabel(lex_State *ls, String *name, int pc, int patch) {
  DataBlk *blk = (DataBlk *)ls->pdata;
  Labeldsc l;
  l.name = name;
  l.patch = patch;
  l.pc = pc;
  if (blk->labels.n >= blk->labels.s)
    cythM_vecgrow(ls->C, blk->labels.labels, blk->labels.s, Vardsc);
  Labeldsc *pl;
  for (cmem_t i = 0; i < blk->labels.n; i++) {
    pl = blk->labels.labels + i;
    if (pl->name == name && pl->patch) {
      patchlab(ls, pl->pc, l);
      *pl = l;
      return;
    } else if (pl->name == name && !pl->patch) {
      cythE_error(ls->C, "Trying to redefine label '%*s'",
                  (unsigned int)name->len, name->data);
    }
  }
  blk->labels.labels[blk->labels.n++] = l;
}

static Labeldsc *getlabel(lex_State *ls, String *name) {
  DataBlk *d = (DataBlk*)ls->pdata;
  for (cmem_t i = 0; i < d->labels.n; i++) {
    if (d->labels.labels[i].name == name)
      return &d->labels.labels[i];
  }
  return NULL;
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

static void openfunc(lex_State *ls, func_State *fs) {
  fs->ls = ls;
  fs->f = cythF_newfunc(ls->C);
  fs->prev = ls->fs;
  ls->fs = fs;
}

static void closefunc(lex_State *ls) {
  func_State *fs = ls->fs->prev;
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
  case TK_TRUE:
    *res = b2obj(1);
    break;
  case TK_FALSE:
    *res = b2obj(0);
    break;
  default:
    cythL_syntaxerror(ls, "Invalid value specifier.");
    break;
  }
}

static int labelref(lex_State *ls) {
  Labeldsc *l;
  String *name;
  name = expect(ls, TK_NAME, " label name").value.s;
  l = getlabel(ls, name);
  if (!l) { /* set to patch it later */
    setlabel(ls, name, relpc(ls), NEEDPATCH);
    return 0;
  }
  return cythC_imm_new(l->pc - relpc(ls) - 1);
}

/* check for any unpatched labels */
static void check_labels(lex_State *ls) {
  DataBlk *d = (DataBlk *)ls->pdata;
  Labeldsc l;
  for (cmem_t i = 0; i < d->labels.n; i++) {
    l = d->labels.labels[i];
    if (l.patch)
      cythE_error(ls->C, "Unknown label '%*s'",
        (unsigned int) l.name->len, l.name->data);
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
    if (v.k == VKLOC) {
      setopcode(i, OP_GETVAR);
      setargz(i, emitK(ls, s2obj(name)));
    } else { /* a function */
      setopcode(i, OP_GETGLB);
      setargz(i, emitK(ls, s2obj(name)));
    }
  } break;
  case TK_EQ: setopcode(i, OP_EQ); break;
  case TK_NEQ: setopcode(i, OP_NEQ); break;
  case TK_JF:
    setopcode(i, OP_JF);
    setargz(i, labelref(ls));
    break;
  case TK_CALL: {
    int nargs = expect(ls, TK_INT,
      "Expected number of arguments"
      " for call instruction.").value.i;
    setopcode(i, OP_CALL);
    setargz(i, nargs);
  } break;
  case TK_JT:
    setopcode(i, OP_JT);
    setargz(i, labelref(ls));
    break;
  case TK_DUP: setopcode(i, OP_DUP); break;
  case TK_SWAP: setopcode(i, OP_SWAP); break;
  case TK_JMP:
    setopcode(i, OP_JMP);
    setargz(i, labelref(ls));
    break;
  case TK_POP: setopcode(i, OP_POP); break;
  default:
    error_unknown(ls, "instruction name");
    break;
  }
  emitC(ls, i, line);
}

/* ':' NAME ':' */
static void label(lex_State *ls) {
  String *name;
  int pc;
  expect(ls, ':', "':' token");
  pc = relpc(ls);
  name = expect(ls, TK_NAME, "valid label name").value.s;
  expect(ls, ':', "':' token");
  setlabel(ls, name, pc, NOPATCH);
}

/* label | instruction */
static void instblock(lex_State *ls) {
  switch (ls->t.type) {
  case ':':
    label(ls);
    break;
  default:
    instruction(ls);
    break;
  }
}

static void blockbody(lex_State *ls, int stop) {
  Symtab s = {0};
  enter(ls, &s);
  while (!match(ls, stop) && !match(ls, TK_EOF)) {
    instblock(ls);
  }
  check_labels(ls);
  leave(ls, s);
}

/* '(' name list ')' */
static void funcparams(lex_State *ls) {
  cmem_t first_var = ls->fs->f->ncode-1;
  cmem_t last_var = first_var;
  int line = 1;
  Instruction i = 0;
  Tvalue name = NONE;
  setopcode(i, OP_SETVAR);
  expect(ls, '(', "Expected '(' to open parameter list");
  Vardsc vd = {0};
  while (ls->t.type != ')' && ls->t.type != TK_EOF) {
    line = saveline(ls);
    name = s2obj(expect(ls, TK_NAME, "Expected parameter name").value.s);
    vd.name = obj2s(&name);
    setargz(i, emitK(ls, name));
    emitC(ls, i, line);
    setvar(ls, vd);
    last_var++;
    ls->fs->f->nparams++;
  }
  expect(ls, ')', "Expected ')' to close parameter list");
  last_var++;
  Instruction *code = ls->fs->f->code;
  Instruction tmp;
  while (first_var < last_var) {
    tmp = code[first_var];
    code[first_var] = code[last_var];
    code[last_var] = tmp;
    first_var++;
    last_var--;
  }
}

/* '->' INT */
static void funcresults(lex_State *ls) {
  expect(ls, TK_ARROW, "Expected '->'");
  int n = expect(ls, TK_INT, "Expected number of function results").value.i;
  ls->fs->f->nresults = n;
  if (ls->fs->f->nresults >= MAXRESULT)
    cythL_syntaxerror(ls, "Function bypasses the maximum number of results allowed");
}

/* '(' func params funcresults instlist ')' */
static void func(lex_State *ls) {
  func_State fs;
  openfunc(ls, &fs);
  cyth_Function *f = ls->fs->f;
  String *name;
  int start_line = saveline(ls);
  expect(ls, '(', "'('");
  expect(ls, TK_FUNC, "'func' keyword.");
  name = expect(ls, TK_NAME, "identifier").value.s;
  funcparams(ls);
  funcresults(ls);
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
  func_State fs;
  openfunc(ls, &fs);
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

/* parse the main function of a chunk */
cyth_Function *cythP_parse_cyth(cyth_State *C, Stream *input, char *chunkname) {
  cyth_Function *f = NULL;
  DataBlk blk = {0};
  lex_State ls = cythL_new(C, LEXMCYTH, chunkname, input);
  ls.pdata = (void*)&blk;
  cythL_next(&ls);
  cythE_runprotected(C, pmainfunc, &ls);
  if (!cythA_popint(C)) {
    /* if it doesn't fails, f is the top of the stack (the main function) */
    f = obj2f(&C->top.p[-1]);
  }
  cythO_buffer_free(C, &ls.buf);
  cythM_vecfree(C, blk.vars.vars, blk.vars.s, Vardsc);
  cythM_vecfree(C, blk.labels.labels, blk.labels.s, Labeldsc);
  return f; /* function is either null or the top */
}

#undef saveline
#undef freturn