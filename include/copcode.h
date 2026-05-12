#ifndef COPCODE_H
#define COPCODE_H
#include <cprefix.h>

/*
** An instruction is an unsigned 32-bit integer
** that can have the following structures:
** |**** ****|**** ****|**** ****|**** ****|
** | opcode  | argZ                        | iZ
** | opcode  | argZ(signed)                | iZs
** | opcode  | argA              | argb    | iAb
** | 0-7     | 8-15    | 16-23   | 24-31   |
*/
typedef uint32_t Instruction;
typedef uint32_t argZ; /* actually 24 bits */

typedef uint8_t argb;
typedef uint16_t argA;

enum opmode {
  iZ,  /* 1 i24       */
  iZs, /* 1 u24       */
  iAb  /* 1 i16, 1 i8 */
};

  /* OP_PUSH    iZ      push(k[z])                      */
  /* OP_POP     iZ      pop()                           */
  /* OP_ADD     iZ      push(pop() + pop())             */
  /* OP_SUB     iZ      push(pop() - pop())             */
  /* OP_DIV     iZ      push(pop() / pop())             */
  /* OP_MUL     iZ      push(pop() * pop())             */
  /* OP_SETVAR  iZ      vars[k[z]] = pop()              */
  /* OP_GETVAR  iZ      push(vars[k[z]])                */
  /* OP_RETURN  iZ      return                          */
  /* OP_EQ      iZ      pop() == pop()                  */
  /* OP_NEQ     iZ      pop() != pop()                  */
  /* OP_JT      iZs     if pop() = true then jmp        */
  /* OP_JF      iZs     if pop() = false then jmp       */
  /* OP_JMP     iZs     pc += z                         */
  /* OP_FUNC    iZ      push(f[Z])                      */
  /* OP_SETGLB  iZ      gt[k[z]] = pop()                */
  /* OP_GETGLB  iZ      push(gt[k[z]])                  */
  /* OP_CALL    iAb     call(-(a+1),b)                  */
  /* OP_DUP     iZ      x=pop() push(x) push(x)         */
  /* OP_SWAP    iZ      x,y=pop(),pop() push(y) push(x) */
  /* OP_COUNT (guard)                                   */
  /* opcode   | mode |  desc                            */
#define OPCODES \
  X(OP_PUSH, "PUSH", iZ)     \
  X(OP_POP, "POP", iZ)       \
  X(OP_ADD, "ADD", iZ)       \
  X(OP_SUB, "SUB", iZ)       \
  X(OP_DIV, "DIV", iZ)       \
  X(OP_MUL, "MUL", iZ)       \
  X(OP_SETVAR, "SETVAR", iZ) \
  X(OP_GETVAR, "GETVAR", iZ) \
  X(OP_RETURN, "RETURN", iZ) \
  X(OP_EQ, "EQ", iZ)         \
  X(OP_NEQ, "NEQ", iZ)       \
  X(OP_JT, "JT", iZs)        \
  X(OP_JF, "JF", iZs)        \
  X(OP_JMP, "JMP", iZs)      \
  X(OP_FUNC, "FUNC", iZ)     \
  X(OP_SETGLB, "SETGLB", iZ) \
  X(OP_GETGLB, "GETGLB", iZ) \
  X(OP_CALL, "CALL", iAb)    \
  X(OP_DUP, "DUP", iZ)       \
  X(OP_SWAP, "SWAP", iZ)

enum opcodes {
#define X(name, string, opmode) name,
  OPCODES
#undef X
  OP_COUNT /* how many opcodes we have */
};

/* masks */

#define MASK0 0xFF
#define MASK1 0xFFFF
#define MASK2 0xFFFFFF

/* positions */

#define OPCODE_POS 0
#define ARGZ_POS 8

#define ARGA_POS 8
#define ARGB_POS 24

/* get and set fields */

#define getfield(i, pos, mask) (((i) >> pos) & mask)
#define getopcode(i) ((i)&MASK0)
#define getargz(i) (getfield(i, ARGZ_POS, MASK2))

#define getarga(i) (getfield(i, ARGA_POS, MASK1))
#define getargb(i) (getfield(i, ARGB_POS, MASK0))

#define setfield(i, v, pos, m) ((i) = ((i) & ~(m << pos)) | ((v & m) << pos))
#define setopcode(i, o) (setfield(i, o, OPCODE_POS, MASK0))
#define setargz(i, z) (setfield(i, z, ARGZ_POS, MASK2))

#define setarga(i, a) (setfield(i, a, ARGA_POS, MASK1))
#define setargb(i, b) (setfield(i, b, ARGB_POS, MASK0))

typedef int32_t Imm;

int cythC_getmode(Instruction i);
char *cythC_getopcode(Instruction i);

/*
** the first bit of i determines if the integer
** is negative or positive
*/
static inline Imm cythC_imm_new(int32_t i) {
  return (Imm)(i < 0 ? (((-i) << 1) + 1) : (i << 1));
}

static inline int32_t cythC_imm2int(Imm imm) {
  return (int32_t)(imm & 0x01 ? (-(imm >> 1)) : (imm >> 1));
}
#endif
