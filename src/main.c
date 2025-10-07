#include <cstate.h>
#include <cstring.h>
#include <cparser.h>
#include <caux.h>

int main(int argc, char **argv) {
  cyth_State *C = cythE_openstate();
  if (argc < 2)
    cythE_error(C, "No arguments provided.");
  cythI_loadfile(C, argv[1]);
  cythF_call(C, -1, 0);
  cythE_closestate(C);
  return 0;
}