#include <cvm.h>
#include <caux.h>
#include <cstring.h>

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

/* execute a cyth call */
void cythV_exec(cyth_State *C, Call_info *ci) {
  cyth_Function *f;
  Instruction i;
  Instruction *pc;
returning:
  f = ci->u.cyth.f;
  pc = f->code;
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
    case OP_SETVAR: break; /* not implemented */
    case OP_GETVAR: break; /* not implemented */
    case OP_RETURN: {
      if (ci->prev != NULL && ci->prev->type != CCALL) {
        cythF_poscall(C);
        ci = C->ci; /* return to caller */
        goto returning;
      } else return;
    } break;
    default:
      cythE_error(C, "Unknown opcode '%d'.", getopcode(i));
    }
  }
}
