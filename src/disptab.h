/*
** dispatch table for the VM main loop
** by default, this is disabled, but just in case
** to enable it, define 'CYTH_USE_COMP_GOTOS', but
** it is only available for some C compilers like gcc
*/

#define vmcase(c) LCASE_##c:
#define vmdispatch(c) goto *disptab[c];
#define vmbreak fetchinst(); vmdispatch(getopcode(i))

#define casename(X) LCASE_##X

static const void *const disptab[OP_COUNT] = {
#define X(name, string, opmode) &&casename(name),
  OPCODES
#undef X
};
