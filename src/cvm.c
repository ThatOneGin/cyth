#include <cvm.h>
#include <cstack.h>
#include <cstring.h>

#define fetchinst() {                     \
  if ((cmem_t)(pc - f->code) >= f->ncode) \
    cythE_error(C,                        \
      "Function has no "                  \
      "return instruction.");             \
  i = *(pc++);}

int cythV_objequ(cyth_State *C,
                 Tvalue t1, Tvalue t2) {
  if (cyth_tt(&t1) != cyth_tt(&t2))
    return 0;
  switch (cyth_tt(&t1)) {
  case CYTH_INTEGER:
    return obj2i(&t1) == obj2i(&t2);
    break;
  case CYTH_STRING:
    return cythS_streq(obj2s(&t1), obj2s(&t2));
    break;
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

/* execute function 'f' until a return instruction */
void cythV_exec(cyth_State *C, cyth_Function *f) {
  Instruction i;
  Instruction *pc = f->code;
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
    case OP_RETURN:
      /*
      ** This should return from current call to the
      ** caller, but we don't have this by now.
      */
      return;
    default:
      cythE_error(C, "Unknown opcode '%d'.", getopcode(i));
    }
  }
}
