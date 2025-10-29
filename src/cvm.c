#include <cvm.h>
#include <caux.h>
#include <cstring.h>
#include <cgc.h>

#define fetchinst() {                     \
  if ((cmem_t)(pc - f->code) >= f->ncode) \
    cythE_error(C,                        \
      "Function has no "                  \
      "return instruction.");             \
  i = *(pc++);                            \
  ci->u.cyth.pc++;}

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
  case CYTH_TABLE:
    return obj2t(&t1) == obj2t(&t2);
  case CYTH_BOOL:
    return obj2b(&t1) == obj2b(&t2);
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
static void getvar(cyth_State *C, Call_info *ci, Tvalue name, Tvalue *res) {
  Call_info *l = ci;
  while (l != NULL) {
    if (l->type != CYTHCALL) {
      l = l->prev;
      continue;
    }
    cythH_get(C, l->u.cyth.locvars, name, res);
    if (res->tt_ == CYTH_NONE) {
      l = l->prev;
      continue;
    } else {
      return;
    }
  }
  *res = NONE;
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
  return obj2b(&dummy);
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

/* execute a cyth call */
void cythV_exec(cyth_State *C, Call_info *ci) {
  cyth_Function *f;
  Instruction i;
  Instruction *pc;
  Table *vars;
returning:
  f = ci->u.cyth.f;
  pc = f->code + ci->u.cyth.pc;
  vars = ci->u.cyth.locvars;
  for (;;) {
    fetchinst();
    switch (getopcode(i)) {
    case OP_PUSH:
      cythA_push(C, getk(f, getargz(i)));
      break;
    case OP_POP:
      cythE_dectop(C);
      break;
    case OP_ADD: {
      Tvalue r = cythA_pop(C);
      Tvalue l = cythA_pop(C);
      cythA_pushint(C, obj2i(&l) + obj2i(&r));
    } break;
    case OP_SETVAR: {
      Tvalue v = C->top[-1];
      cythH_append(C, vars, getk(f, getargz(i)), v);
      cythE_dectop(C);
    } break;
    case OP_GETVAR: {
      getvar(C, ci, getk(f, getargz(i)), C->top);
      cythE_inctop(C);
    } break;
    case OP_RETURN: {
      if (ci->prev != NULL && ci->prev->type != CCALL) {
        cythF_poscall(C);
        ci = C->ci; /* return to caller */
        goto returning;
      } else return;
    } break;
    case OP_EQ: {
      Tvalue r = cythA_pop(C);
      Tvalue l = cythA_pop(C);
      cythA_push(C, b2obj(cythV_objequ(C, l, r)));
    } break;
    case OP_NEQ: {
      Tvalue r = cythA_pop(C);
      Tvalue l = cythA_pop(C);
      cythA_push(C, b2obj((!cythV_objequ(C, l, r))));
    } break;
    case OP_JT: {
      Tvalue val = cythA_pop(C);
      if (cythV_toboolean(val, NULL))
        fetchinst();
    } break;
    case OP_JMP: {
      int32_t az = getargz(i);
      if ((pc-f->code) + az >= (int64_t)f->ncode)
        cythE_error(C, "Invalid jump trying to jump to offset %ld.\n", (pc-f->code) + az);
      else
        pc += az;
    } break;
    case OP_FUNC: {
      cythA_push(C, getf(f, getargz(i)));
    } break;
    case OP_SETGLB: {
      Tvalue k = getk(f, getargz(i));
      if (cyth_tt(&k) != CYTH_STRING)
        cythE_error(C, "Invalid global variable name.\n");
      Tvalue v = cythA_pop(C);
      cythV_setglobal(C, obj2s(&k), v);
    } break;
    case OP_GETGLB: {
      Tvalue k = getk(f, getargz(i));
      if (cyth_tt(&k) != CYTH_STRING)
        cythE_error(C, "Invalid global variable name.\n");
      Tvalue v;
      cythV_getglobal(C, obj2s(&k), &v);
      cythA_push(C, v);
    } break;
    case OP_CALL: {
      int f = -(getargz(i)+1); /* get relative index to the stack */
      cythF_precall(C, &C->top[f], getargz(i)); /* load function */
      C->ci->prev = ci;
      ci = C->ci; /* replace old frame with the new one */
      goto returning; /* but execute in the same C call */
    } break;
    default:
      cythE_error(C, "Unknown opcode '%d'.", getopcode(i));
    }
  }
}
