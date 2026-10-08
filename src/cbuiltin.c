#include <cbuiltin.h>
#include <cstring.h>
#include <cgc.h>
#include <cvm.h>

#define cyth_builtinerr(C, f) cythE_error(C, "%s: " f, __func__)
#define cyth_pushcstr(C, s) (cythA_pushstr(C, cythS_new(C, s)))

/* allocate a string */
static String *alloc_static_string(cyth_State *C, char *data) {
  String *s = cythS_new(C, data);
  cythG_uncoll(C, C->G->list);
  return s;
}

static int tostring(cyth_State *C) {
  static int init = 0;
  static String *none = NULL;
  static String *table = NULL;
  static String *function = NULL;
  static String *userdata = NULL;
  static String *array = NULL;
  if (!init) {
    /* to avoid the overhead of searching strings */
    none = alloc_static_string(C, "none");
    table = alloc_static_string(C, "table");
    function = alloc_static_string(C, "function");
    userdata = alloc_static_string(C, "userdata");
    array = alloc_static_string(C, "array");
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
  case CYTH_CFUNCTION:
  case CYTH_FUNCTION: cythA_pushstr(C, function); break;
  case CYTH_USERDATA: cythA_pushstr(C, userdata); break;
  case CYTH_ARRAY: cythA_pushstr(C, array); break;
  case CYTH_STRING: cythA_pushstr(C, obj2s(&v)); break;
  case CYTH_BOOL: cyth_pushcstr(C, (obj2b(&v) ? "true" : "false")); break;
  case CYTH_INTEGER:
    snprintf(buf, 32, "%ld", obj2i(&v));
    cyth_pushcstr(C, buf);
    break;
  }
  return 1;
}

static int print(cyth_State *C) {
  int n = cythE_gettop(C);
  String *s = NULL;
  for (int i = 0; i < n; i++) {
    cythA_push(C, cythA_arg(C, i+1));
    tostring(C);
    s = cythA_popstr(C);
    printf("%*s\n", (int)s->len, s->data);
  }
  return 0;
}

static int load(cyth_State *C) {
  String *s = cythA_popstr(C);
  cythI_loadfile(C, s2cstr(s));
  cythF_call(C, -1, 0, 0);
  return 0;
}

static int array(cyth_State *C) {
  cythA_push(C, a2obj(cythR_new(C)));
  return 1;
}

static int table(cyth_State *C) {
  cythA_push(C, t2obj(cythH_new(C)));
  return 1;
}

static int set(cyth_State *C) {
  Tvalue k;
  cythV_swap(C);
  k = cythA_pop(C);
  cythV_setf(C, k);
  return 0;
}

static int get(cyth_State *C) {
  Tvalue res, k;
  k = cythA_pop(C);
  cythV_getf(C, &res, k);
  cythA_push(C, res);
  return 1;
}

static cyth_reg funcs[] = {
  {"print", print},
  {"tostring", tostring},
  {"load", load},
  {"array", array},
  {"table", table},
  {"set", set},
  {"get", get},
  {NULL, NULL}
};

void cythB_openlib(cyth_State *C) {
  static int init = 0;
  int i = 0;
  if (init) return;
  init = 1;
  while (funcs[i].func != NULL &&
         funcs[i].name != NULL) {
    cythA_regcf(C, funcs[i].func, funcs[i].name);
    i++;
  }
}