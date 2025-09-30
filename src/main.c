#include <cstate.h>
#include <cstack.h>

int main() {
  cyth_State *C = cythE_openstate();
  for (int i = 0; i < 21; i++) {
    cythA_pushint(C, i);
  }
  for (int i = 0; i < 21; i++) {
    printf("%d\n", cythA_popint(C));
  }
  cythE_closestate(C);
  return 0;
}
