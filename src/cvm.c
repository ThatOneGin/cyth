#include <cvm.h>
#include <caux.h>
#include <cstring.h>
#include <cgc.h>
#include <ctype.h>

#define rebase(C)          \
  {if ((C)->rebase) {      \
    base = ci->func.p + 1; \
    (C)->rebase = 0;}}

/*
** there are some cases which you want to jump
** to a negative address (-1) and those are
** completely safe, due to how jumps are done
** in the main loop, but we also need to check for
** those invalid and negative jumps (less than -1 and greater
** than the size of the instruction list)
*/
#define checkjmp(pc, f, target) \
  if ((pc - f->code) + target >= (int64_t)f->ncode || (pc - f->code) < -1)\
    cythE_error(f->C, "Trying to make an out-of-bounds jump (pc=%lu, target=%u)",\
      (pc - f->code), target)

#define dojmp(az)        \
  {checkjmp(pc, f, az);  \
   pc += az;             \
   ci->u.cyth.pc += az;}

#define fetchinst() {                     \
  if ((cmem_t)(pc - f->code) >= f->ncode) \
    cythE_error(C,                        \
      "Function has no "                  \
      "return instruction.");             \
  i = *(pc++);                            \
  ci->u.cyth.pc++;                        \
  rebase(C)}

#if !defined(CYTH_USE_COMP_GOTOS)
#  define vmdispatch(x) switch (x)
#  define vmcase(x) case x:
#  define vmbreak break
#endif

int cythV_objequ(cyth_State *C,
                 Tvalue t1, Tvalue t2) {
  (void)C;
  if (cyth_tt(&t1) != cyth_tt(&t2))
    return 0;
  switch (cyth_tt(&t1)) {
  case CYTH_INTEGER:
    return obj2i(&t1) == obj2i(&t2);
  case CYTH_STRING:
    return cythS_streq(obj2s(&t1), obj2s(&t2));
  case CYTH_FUNCTION:
    return obj2f(&t1) == obj2f(&t2);
  case CYTH_CFUNCTION:
    return obj2cf(&t1) == obj2cf(&t2);
  case CYTH_TABLE:
    return obj2t(&t1) == obj2t(&t2);
  case CYTH_BOOL:
    return obj2b(&t1) == obj2b(&t2);
  case CYTH_USERDATA:
    return obj2ud(&t1).data == obj2ud(&t2).data;
  default:
    return 0;
  }
  return 0;
}

static Tvalue getk(cyth_Function *f, argZ az) {
  if (az >= f->nk)
    cythE_error(f->C, "Unreachable constant value (address=%u)", az);
  return f->k[az];
}

static Tvalue getf(cyth_Function *f, argZ az) {
  if (az >= f->nf)
    cythE_error(f->C, "Unreachable closure (address=%u)", az);
  return f2obj(f->f[az]);
}

/* get a local variable (checks for outer environments) */
static void getvar(cyth_State *C, Call_info *ci, argZ key, Tvalue *res) {
  if ((argZ)ci->u.cyth.locvars->narray < key)
    cythE_error(C, "invalid variable index %u.\n", key);
  cythR_get(C, ci->u.cyth.locvars, (cyth_integer)key, res);
}

/* transform value to a boolean */
int cythV_toboolean(Tvalue v, Tvalue *res) {
  Tvalue dummy;
  if (res == NULL)
    res = &dummy;
  switch (cyth_tt(&v)) {
  case CYTH_INTEGER:
  case CYTH_STRING:
  case CYTH_FUNCTION:
  case CYTH_TABLE:
    *res = b2obj(1);
    break;
  case CYTH_BOOL:
    *res = v;
    break;
  case CYTH_NONE:
  default:
    *res = b2obj(0);
    break;
  }
  return obj2b(res);
}

void cythV_setglobal(cyth_State *C, String *name, Tvalue v) {
  Tvalue k = s2obj(name);
  cythH_append(C, C->gt, k, v);
}

int cythV_getglobal(cyth_State *C, String *name, Tvalue *res) {
  Tvalue k = s2obj(name);
  cythH_get(C, C->gt, k, res);
  return (res->tt_ != CYTH_NONE);
}

/*
** Custom pop, we assume that the current Call_info's func slot
** is the stack base to avoid functions to access values
** that are outside their frame (arguments are an exception)
*/
static Tvalue pop(cyth_State *C) {
  stkrel base = C->ci->func.p + 1;
  if (C->top.p == base) {
    /* For now it is better to raise an error */
    cythE_error(C, "Stack underflow.\n");
  } else {
    return cythA_pop(C);
  }
  return NONE; /* avoid gcc's warning, but this is unreachable */
}

void cythV_dup(cyth_State *C) {
  stkrel base = C->ci->func.p + 1;
  if (C->top.p == base)
    cythE_error(C, "Stack underflow.\n");
  objcopy(C->top.p, &C->top.p[-1]);
  cythE_inctop(C);
}

void cythV_swap(cyth_State *C) {
  stkrel base = C->ci->func.p + 1;
  if (C->top.p - base <= 1)
    cythE_error(C, "Stack underflow.\n");
  stkrel tmp = C->top.p-1;
  objcopy(C->top.p-1, C->top.p - 2);
  objcopy(C->top.p-2, tmp);
}

/*
** try to convert value to an integer,
** currently, only strings and integers can be converted
*/
static int tointeger(Tvalue t, cyth_integer *i) {
  switch (t.tt_) {
  case CYTH_INTEGER:
    *i = obj2i(&t);
    break;
  case CYTH_STRING: {
    String *s = obj2s(&t);
    for (cmem_t j = 0; j < s->len; j++) {
      if (!isdigit(s->data[j])) {
        *i = 0;
        return -1;
      }
      *i = (*i * 10) + (s->data[j] - '0');
    }
  } break;
  default:
    return -1;
  }
  return 0;
}

/* convert both hands to integers and do the operation */
static int dobinop(cyth_integer *res, int op, Tvalue l, Tvalue r) {
  cyth_integer lhs = 0;
  cyth_integer rhs = 0;
  int lres = tointeger(l, &lhs);
  int rres = tointeger(r, &rhs);
  if (lres < 0 || rres < 0)
    return -1;
  switch (op) {
  case OPR_ADD: *res = (cyth_integer)lhs + rhs; break;
  case OPR_SUB: *res = (cyth_integer)lhs - rhs; break;
  case OPR_MUL: *res = (cyth_integer)lhs * rhs; break;
  case OPR_DIV: *res = (cyth_integer)lhs / rhs; break;
  default: cyth_assert(0); break;
  }
  return 0;
}

int cythV_arith(cyth_State *C, Tvalue *res, Tvalue l, Tvalue r, int op) {
  (void)C;
  *res = i2obj(0);
  if (dobinop(&obj2i(res), op, l, r) < 0)
    return 0;
  return 1;
}

/* get the field of a table or an array */
void cythV_getf(cyth_State *C, Tvalue *res, Tvalue k)
{
  /* top[-1] = table/array
  ** k = key/field
  */
  Tvalue t = cythA_pop(C);
  switch (t.tt_) {
  case CYTH_ARRAY:
    if (k.tt_ != CYTH_INTEGER)
      cythE_error(C,
        "an array only accepts integer keys");
    cythR_get(C, obj2a(&t), obj2i(&k), res);
    break;
  case CYTH_TABLE:
    cythH_get(C, obj2t(&t), k, res);
    break;
  default:
    cythE_error(C,
      "trying to get the value of an "
      "invalid kind of target (not an array nor object)");
  }
}

/* set the field of a table or an array */
void cythV_setf(cyth_State *C, Tvalue k)
{
  /* top[-1] = value to set
  ** top[-2] = table/array
  ** k = key/field
  */
  Tvalue v, t;
  v = cythA_pop(C);
  t = cythA_pop(C);
  switch (t.tt_) {
  case CYTH_ARRAY:
    if (k.tt_ != CYTH_INTEGER)
      cythE_error(C,
        "an array only accepts integer keys");
    cythR_push(C, obj2a(&t), obj2i(&k), v);
    break;
  case CYTH_TABLE:
    cythH_append(C, obj2t(&t), k, v);
    break;
  default:
    cythE_error(C,
      "trying to set the value of an "
      "invalid kind of target (not an array nor object)");
  }
}

/* execute a cyth call */
void cythV_exec(cyth_State *C, Call_info *ci) {
  cyth_Function *f;
  Instruction i;
  Instruction *pc;
  Array *vars;
  stkrel base;
#if defined(CYTH_USE_COMP_GOTOS)
#include "disptab.h"
#endif
returning:
  f = ci->u.cyth.f;
  pc = f->code + ci->u.cyth.pc;
  vars = ci->u.cyth.locvars;
  base = ci->func.p + 1;
  for (;;) {
    fetchinst();
    vmdispatch (getopcode(i)) {
      vmcase(OP_PUSH) {
        cythA_push(C, getk(f, getargz(i)));
      } vmbreak;
      vmcase(OP_POP) {
        cythE_dectop(C);
      } vmbreak;
      vmcase(OP_BINOP) {
        cyth_integer res = 0;
        Tvalue r = pop(C);
        Tvalue l = pop(C);
        if (dobinop(&res, getargz(i), l, r) < 0)
          cythE_error(C, "could not do binary operation");
        cythA_pushint(C, res);
      } vmbreak;
      vmcase(OP_SETVAR) {
        if (C->top.p == base)
          cythE_error(C, "No available value in "
                         "the stack for variable.\n");
        Tvalue v = C->top.p[-1];
        cythR_push(C, vars, getargz(i), v);
        cythE_dectop(C);
      } vmbreak;
      vmcase(OP_GETVAR) {
        getvar(C, ci, getargz(i), C->top.p);
        cythE_inctop(C);
      } vmbreak;
      vmcase(OP_RETURN) {
        if (ci->prev != NULL && ci->prev->type != CCALL) {
          cythF_poscall(C, f->nresults, ci->u.cyth.callnres);
          ci = C->ci; /* return to caller */
          goto returning;
        } else return;
      } vmbreak;
      vmcase(OP_EQ) {
        Tvalue r = pop(C);
        Tvalue l = pop(C);
        cythA_push(C, b2obj(cythV_objequ(C, l, r)));
      } vmbreak;
      vmcase(OP_NEQ) {
        Tvalue r = pop(C);
        Tvalue l = pop(C);
        cythA_push(C, b2obj((!cythV_objequ(C, l, r))));
      } vmbreak;
      vmcase(OP_JT) {
        int32_t az = cythC_imm2int(getargz(i));
        Tvalue val = pop(C);
        if (cythV_toboolean(val, NULL))
          dojmp(az);
      } vmbreak;
      vmcase(OP_JF) {
        int32_t az = cythC_imm2int(getargz(i));
        Tvalue val = pop(C);
        if (!cythV_toboolean(val, NULL))
          dojmp(az);
      } vmbreak;
      vmcase(OP_JMP) {
        int32_t az = cythC_imm2int(getargz(i));
        dojmp(az);
      } vmbreak;
      vmcase(OP_FUNC) {
        cythA_push(C, getf(f, getargz(i)));
      } vmbreak;
      vmcase(OP_SETGLB) {
        Tvalue k = getk(f, getargz(i));
        if (cyth_tt(&k) != CYTH_STRING)
          cythE_error(C, "Invalid global variable name.\n");
        Tvalue v = pop(C);
        cythV_setglobal(C, obj2s(&k), v);
      } vmbreak;
      vmcase(OP_GETGLB) {
        Tvalue k = getk(f, getargz(i));
        if (cyth_tt(&k) != CYTH_STRING)
          cythE_error(C, "Invalid global variable name.\n");
        Tvalue v;
        cythV_getglobal(C, obj2s(&k), &v);
        if (cyth_tt(&v) == CYTH_NONE)
          cythE_error(C, "Unknown global variable '%s'.\n", s2cstr(obj2s(&k)));
        cythA_push(C, v);
      } vmbreak;
      vmcase(OP_CALL) {
        int f = -(getarga(i)+1); /* get relative index to the stack */
        stkrel func = &C->top.p[f];
        if (func < base)
          cythE_error(C, "No function on the stack to call.");
        cythF_precall(C, func, getarga(i), getargb(i)); /* load function */
        C->ci->prev = ci;
        ci = C->ci; /* replace old frame with the new one */
        if (ci->type == CYTHCALL)
          goto returning;
        else {
          cyth_Cfunction cf = obj2cf(func);
          int cnres = cf(C);
          cythF_poscall(C, cnres, getargb(i));
          ci = C->ci;
        }
      } vmbreak;
      vmcase(OP_DUP) {
        cythV_dup(C);
      } vmbreak;
      vmcase(OP_SWAP) {
        cythV_swap(C);
      } vmbreak;
      vmcase(OP_GETF) {
        Tvalue res;
        cythV_getf(C, &res, getk(f, getargz(i)));
        cythA_push(C, res);
      } vmbreak;
      vmcase(OP_SETF) {
        cythV_setf(C, getk(f, getargz(i)));
      } vmbreak;
    }
  }
}
