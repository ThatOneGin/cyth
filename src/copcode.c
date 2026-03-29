#include <copcode.h>


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
