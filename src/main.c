#include <cstate.h>
#include <cstring.h>
#include <cparser.h>
#include <cstack.h>

int main() {
  cyth_State *C = cythE_openstate();
  cythP_parse(C,
    "(func main()"
    "  const 12;"
    "  const 21;"
    "  add;"
    "  return;)", "main");
  cythF_call(C, -1, 0);
  printf("%d\n", cythA_popint(C));
  cythE_closestate(C);
  return 0;
}
