#include <cstate.h>
#include <cstack.h>
#include <cstring.h>
#include <cfunc.h>
#include <copcode.h>
#include <cvm.h>

int main() {
  cyth_State *C = cythE_openstate();
  cyth_Function *f = cythF_newfunc(C);

  Instruction push = 0;
  Instruction add = 0;
  Instruction ret = 0;

  setargz(push, cythF_emitK(f, i2obj(32)));
  setopcode(push, OP_PUSH);
  setopcode(add, OP_ADD);
  setopcode(ret, OP_RETURN);

  cythF_emitC(f, push, -1); /* push 32 */
  cythF_emitC(f, push, -1); /* push 32 */
  cythF_emitC(f, add,  -1); /* add     */
  cythF_emitC(f, ret,  -1); /* ret     */

  cythV_exec(C, f);

  printf("%d\n", cythA_popint(C));

  cythF_freefunc(f);
  cythE_closestate(C);
  return 0;
}
