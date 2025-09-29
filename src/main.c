#include <cstate.h>

int main() {
  cyth_State *C = cythE_openstate();
  cythE_closestate(C);
  return 0;
}
