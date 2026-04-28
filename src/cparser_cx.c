#include <cparser.h>
#include <clex_cx.h>
#include <cio.h>
#include <cmem.h>
#include <caux.h>

#define ensure(ls, e, msg) ((e) ? ((void)0) : cythL_syntaxerror(ls, msg))
#define saveline(ls) ((ls)->line)
#define freturn(ls, line) emitC(ls, (OP_RETURN<<OPCODE_POS), line)
#define emitInstl(ls, line, opcode, argz) emitInst_(ls, line, opcode, argz)
#define emitInst(ls, opcode, argz) emitInst_(ls, -1, opcode, argz)

/* types */

enum BINOPR {
  OPR_ADD,
  OPR_SUB,
  OPR_DIV,
  OPR_MUL,
  OPR_INVALID
};

enum expdsck {
  EXPINT,  /* e->u.i    */
  EXPSTR,  /* e->u.s    */
  EXPBOOL, /* e->u.info */
  EXPNAME, /* e->u.s    */
  EXPCALL, /* e->u.info */
  EXPUSED, /* e->u.info */
};

typedef struct {
  int k;
  union {
    cyth_integer i;
    String *s;
    int info;
  } u;
} expdsc;

/* error functions */

static void error_expect(lex_State *ls, const char *what) {
  char buf[BUFFERSIZE];
  snprintf(buf, BUFFERSIZE, "Unexpected token (expected %s).", what);
  cythL_syntaxerror(ls, buf);
}

/* auxiliary functions */

static int emitC(lex_State *ls, Instruction i, int line) {
  cyth_Function *f = ls->fs->f;
  return cythF_emitC(f, i, line);
}

static int emitK(lex_State *ls, Tvalue k) {
  cyth_Function *f = ls->fs->f;
  return cythF_emitK(f, k);
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

static void emitfunction(lex_State *ls, int f, int line) {
  Instruction inst = 0;
  setopcode(inst, OP_FUNC);
  setargz(inst, f);
  emitC(ls, inst, line);
}

static int emitInst_(lex_State *ls, int line, int opcode, int argz) {
  Instruction i = 0;
  setopcode(i, opcode);
  setargz(i, argz);
  return emitC(ls, i, (line == -1) ? saveline(ls) : line);
}

static void free_exp(lex_State *ls, expdsc *e) {
  int i = 0;
  switch (e->k) {
  case EXPINT:
    i = emitInst(ls, OP_PUSH, emitK(ls, i2obj(e->u.i)));
    break;
  case EXPSTR:
    i = emitInst(ls, OP_PUSH, emitK(ls, s2obj(e->u.s)));
    break;
  case EXPBOOL:
    i = emitInst(ls, OP_PUSH, emitK(ls, b2obj(e->u.info)));
    break;
  case EXPNAME: {
    Vardsc v;
    getvar(ls, e->u.s, &v);
    int opc = v.k == VKGLB ? OP_GETGLB : OP_GETVAR;
    i = emitInst(ls, opc, emitK(ls, s2obj(e->u.s)));
  } break;
  case EXPCALL:
    break;
  case EXPUSED:
    return;
  default:
    cyth_assert(0);
    break;
  }
  e->k = EXPUSED;
  e->u.info = i;
}

static void expbin(lex_State *ls, int op, expdsc *e1, expdsc *e2) {
  free_exp(ls, e1);
  free_exp(ls, e2);
  switch (op) {
  case OPR_ADD: emitInst(ls, OP_ADD, 0); break;
  case OPR_SUB: emitInst(ls, OP_SUB, 0); break;
  case OPR_DIV: emitInst(ls, OP_DIV, 0); break;
  case OPR_MUL: emitInst(ls, OP_MUL, 0); break;
  default: cyth_assert(0); break;
  }
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

/* parsing functions */

static void expr(lex_State *ls, expdsc *e);
static int explist(lex_State *ls, expdsc *e);

/* pexp = INT | STR | NAME | BOOL */
static void pexp(lex_State *ls, expdsc *e) {
  Token t = next(ls);
  switch (t.type) {
  case TK_INT:
    e->k = EXPINT;
    e->u.i = t.value.i;
    break;
  case TK_STR:
    e->k = EXPSTR;
    e->u.s = t.value.s;
    break;
  case TK_NAME:
    e->k = EXPNAME;
    e->u.s = t.value.s;
    break;
  case TK_TRUE:
  case TK_FALSE:
    e->k = EXPBOOL;
    e->u.info = t.type == TK_TRUE;
    break;
  default:
    error_expect(ls, "expression");
    break;
  }
}

static int prec[] = {
  [OPR_MUL] = 2, [OPR_DIV] = 2, /* *, / */
  [OPR_ADD] = 1, [OPR_SUB] = 1, /* +, - */
  [OPR_INVALID] = 0
};

static int getopr(lex_State *ls) {
  switch (ls->t.type) {
  case '+': return OPR_ADD;
  case '-': return OPR_SUB;
  case '/': return OPR_DIV;
  case '*': return OPR_MUL;
  default: return OPR_INVALID;
  }
}

/* xexp = pexp | pexp '(' explist ')'*/
static void xexp(lex_State *ls, expdsc *e) {
  pexp(ls, e);
  while (1) {
    switch (ls->t.type) {
    case '(': {
      next(ls);
      free_exp(ls, e);
      int n = 0;
      if (ls->t.type != ')')
        n = explist(ls, e);
      expect(ls, ')', "')' to close argument list");
      emitInst(ls, OP_CALL, n);
      e->k = EXPCALL;
      e->u.info = n;
    } break;
    default:
      return;
    }
  }
}

/* bexp = xexp | xexp BINOPR bexp */
static int bexp(lex_State *ls, expdsc *e1, int lim) {
  expdsc e2;
  xexp(ls, e1);
  int opr = getopr(ls);
  int nextop = 0;
  while (opr != OPR_INVALID && prec[opr] > lim) {
    next(ls);
    nextop = bexp(ls, &e2, opr);
    expbin(ls, opr, e1, &e2);
    opr = nextop;
  }
  return opr;
}

/* expr = bexp */
static void expr(lex_State *ls, expdsc *e) {
  bexp(ls, e, 0);
}

/* explist = expr | expr ',' */
static int explist(lex_State *ls, expdsc *e) {
  int i = 1;
  expr(ls, e);
  free_exp(ls, e);
  while (ls->t.type == ',') {
    expr(ls, e);
    free_exp(ls, e);
    ++i;
  }
  return i;
}

/* ret = 'return' expr */
static void ret(lex_State *ls) {
  expdsc e;
  expect(ls, TK_RETURN, "Expected 'return' keyword");
  expr(ls, &e);
  free_exp(ls, &e);
  ls->fs->f->nresults = 1;
  freturn(ls, saveline(ls));
}

/* assign = xexp '=' expr */
static void assign(lex_State *ls, expdsc *e) {
  Vardsc v;
  String *name;
  name = e->u.s;
  getvar(ls, name, &v);
  expect(ls, '=', "'='");
  expr(ls, e);
  free_exp(ls, e);
  int k = emitK(ls, s2obj(name));
  emitInst(ls, OP_SETVAR, k);
  v.k = VKLOC;
  setvar(ls, v);
}

/* exprstat = call | assign */
static void exprstat(lex_State *ls) {
  expdsc e;
  xexp(ls, &e);
  if (ls->t.type == '=') {
    ensure(ls, e.k == EXPNAME, "invalid left-hand on assignment");
    assign(ls, &e);
  } else {
    ensure(ls, e.k == EXPCALL, "expected statement");
    free_exp(ls, &e);
  }
}

static void stat(lex_State *ls) {
  switch (ls->t.type) {
  case TK_RETURN:
    ret(ls);
    break;
  default:
    exprstat(ls);
    break;
  }
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

/* funcbody = 'do' stat list 'end' */
static void funcbody(lex_State *ls) {
  expect(ls, TK_DO, "do-end block");
  while (ls->t.type != TK_EOF && ls->t.type != TK_END) {
    stat(ls);
  }
  expect(ls, TK_END, "end keyword");
}

/* func = 'func' NAME funcparams funcbody */
static void func(lex_State *ls) {
  func_State fs;
  openfunc(ls, &fs);
  cyth_Function *f = ls->fs->f;
  String *name;
  int start_line = saveline(ls);
  expect(ls, TK_FUNC, "'func' keyword.");
  name = expect(ls, TK_NAME, "identifier").value.s;
  funcparams(ls);
  funcbody(ls);
  int end_line = saveline(ls);
  freturn(ls, end_line); /* last instruction (return) */
  closefunc(ls);
  int fidx = cythF_emitF(ls->fs->f, f);
  emitfunction(ls, fidx, end_line);
  setglobal(ls, name, start_line);
  Vardsc var = {0};
  var.k = VKFUN;
  var.i = fidx;
  var.name = name;
  setvar(ls, var);
}

static void mainfunc(lex_State *ls) {
  func_State fs = {0};
  openfunc(ls, &fs);
  cyth_Function *f = fs.f;
  while (ls->t.type != TK_EOF) {
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

cyth_Function *cythP_parse_cx(cyth_State *C, Stream *input, char *chunkname) {
  cyth_Function *f = NULL;
  DataBlk blk = {0};
  lex_State ls = cythL_new(C, LEXMCX, chunkname, input);
  ls.pdata = (void*)&blk;
  cythL_next(&ls);
  cythE_runprotected(C, pmainfunc, &ls);
  if (!cythA_popint(C)) {
    /* if it doesn't fails, f is the top of the stack (the main function) */
    f = obj2f(cythE_peek(C, -1));
  }
  cythO_buffer_free(C, &ls.buf);
  cythM_vecfree(C, blk.vars.vars, blk.vars.s, Vardsc);
  cythM_vecfree(C, blk.labels.labels, blk.labels.s, Labeldsc);
  return f; /* function is either null or the top */
}
