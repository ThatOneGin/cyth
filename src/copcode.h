#ifndef COPCODE_H
#define COPCODE_H
#include <cprefix.h>

/*
** An instruction is an unsigned 32-bit integer
** that can have the following structures:
** |**** ****|**** ****|**** ****|**** ****|
** | opcode  | argZ                        | iZ
** | opcode  | argZ(signed)                | iZs
** | 0-7     | 8-31                        |
*/
typedef uint32_t Instruction;
typedef uint32_t argZ; /* actually 24 bits */

enum opmode {
  iZ, /* z is an unsigned integer */
  iZs /* z is a signed integer */
};

enum opcode {
/* opcode     | mode | desc                       */
  OP_PUSH,   /* iZ      push(k[z])                */
  OP_POP,    /* iZ      pop()                     */
  OP_ADD,    /* iZ      push(pop() + pop())       */
  OP_SETVAR, /* iZ      vars[k[z]] = pop()        */
  OP_GETVAR, /* iZ      push(vars[k[z]])          */
  OP_RETURN, /* iZ      return pop()              */
  OP_EQ,     /* iZ      pop() == pop()            */
  OP_NEQ,    /* iZ,     pop() != pop()            */
  OP_JT,     /* iZ      if pop() = true then pc++ */
  OP_JMP,    /* iZ      pc += z                   */
  OP_FUNC,   /* iZ      push(f[Z])                */
  OP_SETGLB, /* iZ      gt[k[z]] = pop()          */
  OP_GETGLB, /* iz      push(gt[k[z]])            */
  OP_COUNT
};

/* masks */

#define MASK0 0xFF
#define MASK1 0xFFFF
#define MASK2 0xFFFFFF

/* positions */

#define OPCODE_POS 0
#define ARGZ_POS 8

/* get and set fields */

#define getfield(i, pos, mask) (((i) >> pos) & mask)
#define getopcode(i) ((i)&MASK0)
#define getargz(i) (getfield(i, ARGZ_POS, MASK2))

#define setfield(i, v, pos, m) ((i) = ((i) & ~(m << pos)) | ((v & m) << pos))
#define setopcode(i, o) setfield(i, o, OPCODE_POS, MASK0)
#define setargz(i, z) setfield(i, z, ARGZ_POS, MASK2)

int cythC_getmode(Instruction i);
char *cythC_getopcode(Instruction i);
#endif
