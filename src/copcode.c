#include <copcode.h>

byte opmodes[] = {
  [OP_PUSH] = iZ,
  [OP_POP] = iZ,
  [OP_ADD] = iZ,
  [OP_SETVAR] = iZ,
  [OP_GETVAR] = iZ
};

char *opcodes[] = {
  [OP_PUSH] = "PUSH",
  [OP_POP] = "POP",
  [OP_ADD] = "ADD",
  [OP_SETVAR] = "SETVAR",
  [OP_GETVAR] = "GETVAR"
};

int cythC_getmode(Instruction i) {
  if (getopcode(i) >= OP_COUNT)
    return -1;
  return opmodes[getopcode(i)];
}

char *cythC_getopcode(Instruction i) {
  if (getopcode(i) >= OP_COUNT)
    return "unknown";
  return opcodes[getopcode(i)];
}
