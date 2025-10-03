#include <cstate.h>
#include <cstack.h>
#include <cstring.h>
#include <cfunc.h>
#include <copcode.h>
#include <cvm.h>
#include <cgc.h>
#include <cmem.h>

int main() {
  cyth_State *C = cythE_openstate();
  Table *t = cythH_new(C);
  cythH_append(C, t, i2obj(0), i2obj(32));
  Tvalue res;
  cythH_get(C, t, i2obj(0), &res);
  printf("%ld\n", res.v.integer);
  cythE_closestate(C);
  return 0;
}
