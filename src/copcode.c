#include <copcode.h>

byte opmodes[] = {
  [OP_PUSH] = iZ,
  [OP_POP] = iZ,
  [OP_ADD] = iZ,
  [OP_SETVAR] = iZ,
  [OP_GETVAR] = iZ,
  [OP_EQ] = iZ,
  [OP_NEQ] = iZ,
  [OP_JMP] = iZs,
  [OP_FUNC] = iZ,
  [OP_CALL] = iZ,
  [OP_DUP] = iZ,
  [OP_SWAP] = iZ
};

char *opcodes[] = {
  [OP_PUSH] = "PUSH",
  [OP_POP] = "POP",
  [OP_ADD] = "ADD",
  [OP_SETVAR] = "SETVAR",
  [OP_GETVAR] = "GETVAR",
  [OP_RETURN] = "RETURN",
  [OP_EQ] = "EQ",
  [OP_NEQ] = "NEQ",
  [OP_JT] = "JT",
  [OP_JMP] = "JMP",
  [OP_FUNC] = "FUNC",
  [OP_SETGLB] = "SETGLB",
  [OP_GETGLB] = "GETGLB",
  [OP_CALL] = "CALL",
  [OP_DUP] = "DUP",
  [OP_SWAP] = "SWAP"
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
