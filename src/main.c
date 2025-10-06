#include <cstate.h>
#include <cstring.h>
#include <clex.h>

int main() {
  cyth_State *C = cythE_openstate();
  lex_State *ls = cythL_new(C, "main", "(func main())");
  for (int i = 0; i < 7; i++) {
    cythL_next(ls);
    if (ls->t.type >= FIRSTRESERVED)
      if (ls->t.type == TK_INT) printf("Integer %ld\n", ls->t.value.i);
      else printf("Word \"%s\"\n", s2cstr(ls->t.value.s));
    else
      if (ls->t.type == TK_EOF) printf("EOF\n");
      else printf("Char %c\n", ls->t.type);
  }
  cythL_free(ls);
  cythE_closestate(C);
  return 0;
}
