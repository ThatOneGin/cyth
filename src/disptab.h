/* dispatch table for the VM main loop */

#define vmcase(c) LCASE_##c:
#define vmdispatch(c) goto *disptab[c];
#define vmbreak fetchinst(); vmdispatch(getopcode(i))

#define casename(x) LCASE_##X

static const void *const disptab[OP_COUNT] = {
  &&casename(OP_PUSH),
  &&casename(OP_POP),
  &&casename(OP_ADD),
  &&casename(OP_SETVAR),
  &&casename(OP_GETVAR),
  &&casename(OP_RETURN),
  &&casename(OP_EQ),
  &&casename(OP_NEQ),
  &&casename(OP_JT),
  &&casename(OP_JMP),
  &&casename(OP_FUNC),
  &&casename(OP_SETGLB),
  &&casename(OP_GETGLB),
  &&casename(OP_CALL),
  &&casename(OP_DUP),
  &&casename(OP_SWAP),
};
