#include <copcode.h>
#include <cvm.h>

int cythC_getmode(Instruction i) {
  switch (getopcode(i)) {
#define X(name, string, opmode) case name: return opmode;
    OPCODES
#undef X
  default: return -1;
  }
}

char *cythC_getopcode(Instruction i) {
  switch(getopcode(i)) {
#define X(name, string, opmode) case name: return string;
    OPCODES
#undef X
  default: return "unknown";
  }
}

char *cythC_getbinopname(int o) {
  switch (o) {
  case OPR_ADD: return "ADD";
  case OPR_SUB: return "SUB";
  case OPR_MUL: return "MUL";
  case OPR_DIV: return "DIV";
  case OPR_EQ: return "EQ";
  case OPR_NE: return "NE";
  case OPR_GT: return "GT";
  case OPR_LT: return "LT";
  default: return "invalid";
  }
}