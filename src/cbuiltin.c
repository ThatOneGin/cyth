#include <cbuiltin.h>
#include <cstring.h>

#define cyth_builtinerr(C, f) cythE_error(C, "%s: " f, __func__)
#define cyth_pushcstr(C, s) (cythA_pushstr(C, cythS_new(C, s)))

int tostring(cyth_State *C) {
  static int init = 0;
  static String *none = NULL;
  static String *table = NULL;
  static String *function = NULL;
  static String *userdata = NULL;
  if (!init) {
    /* to avoid the overhead of searching strings */
    none = cythS_new(C, "none");
    table = cythS_new(C, "table");
    function = cythS_new(C, "function");
    userdata = cythS_new(C, "userdata");
    init = 1;
  }
  if (cythE_gettop(C) == 0)
    cyth_builtinerr(C, "Expected value");
  Tvalue v;
  char buf[32];
  v = cythA_pop(C);
  /* we should just use a table for that */
  switch (v.tt_) {
  case CYTH_NONE: cythA_pushstr(C, none); break;
  case CYTH_TABLE: cythA_pushstr(C, table); break;
  case CYTH_FUNCTION: cythA_pushstr(C, function); break;
  case CYTH_USERDATA: cythA_pushstr(C, userdata); break;
  case CYTH_STRING: cythA_pushstr(C, obj2s(&v)); break;
  case CYTH_BOOL: cyth_pushcstr(C, (obj2b(&v) ? "true" : "false")); break;
  case CYTH_INTEGER:
    snprintf(buf, 32, "%ld", obj2i(&v));
    cyth_pushcstr(C, buf);
    break;
  }
  return 1;
}

int print(cyth_State *C) {
  String *s = NULL;
  tostring(C);
  s = cythA_popstr(C);
  printf("%*s\n", (int)s->len, s->data);
  return 0;
}

static struct {
  char *name;
  cyth_Cfunction f;
} funcs[] = {
  {"print", print},
  {"tostring", tostring},
  {NULL, NULL}
};

void cythB_openlib(cyth_State *C) {
  static int init = 0;
  int i = 0;
  if (init) return;
  init = 1;
  while (funcs[i].f != NULL &&
         funcs[i].name != NULL) {
    cythA_regcf(C, funcs[i].f, funcs[i].name);
    i++;
  }
}