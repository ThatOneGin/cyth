#ifndef COBJECT_H
#define COBJECT_H
#include <cprefix.h>

typedef struct cyth_State cyth_State;
typedef struct global_State global_State;
typedef struct gc_object gc_object;

typedef struct {
  char *data;
  size_t len;
  sbyte aux; /* this field helps to identify reserved words */
} String;

/*
** A 'Table' is an associative array,
** that is, every value can be used as keys.
*/
typedef struct Table Table;
typedef struct Array Array;

#define VALUES \
  X(CYTH_NONE, "none", 0) \
  X(CYTH_INTEGER, "integer", 1) \
  X(CYTH_STRING, "string", 2) \
  X(CYTH_TABLE, "table", 3) \
  X(CYTH_FUNCTION, "function", 4) \
  X(CYTH_USERDATA, "userdata", 5) \
  X(CYTH_BOOL, "bool", 6) \
  X(CYTH_ARRAY, "array", 7)

enum CYTH_VALUES {
#define X(name, str, value) name = value,
  VALUES
#undef X
};

#define NONE ((Tvalue){.tt_=CYTH_NONE,{0}})

typedef struct cyth_Function cyth_Function;
typedef int (*cyth_Cfunction)(cyth_State *);
typedef void (*cyth_Destructor)(void*, void*); /* params: userdata, pointer */

/* types of user data */
#define UDFUN 0
#define UDVAL 1

typedef struct {
  byte type;
  union {
    cyth_Cfunction cfunc;
    struct {
      void *data;
      cmem_t size;
      gc_object *ref;
    } val;
  } u;
  cyth_Destructor destructor;
} userdata;

typedef union {
  cyth_integer integer;
  String *string;
  Table *table;
  cyth_Function *function;
  userdata userdata;
  Array *array;
  byte boolean;
} Value;

typedef struct {
  byte tt_;
  Value v;
} Tvalue;

typedef struct Node {
  Tvalue key;
  Tvalue val;
  struct Node *next;
} Node;

struct Table {
  Node *list;
  int len;
};

struct Array {
  Tvalue *data;
  cmem_t narray;
  cmem_t arraysize;
};

#define cyth_tt(o) ((o)->tt_)
#define obj2i(o) (o)->v.integer
#define obj2s(o) (o)->v.string
#define obj2t(o) (o)->v.table
#define obj2f(o) (o)->v.function
#define obj2ud(o) (o)->v.userdata
#define obj2b(o) (o)->v.boolean
#define obj2a(o) (o)->v.array
#define objcopy(s1, s2) {\
  cyth_tt(s1) = cyth_tt(s2); \
  (s1)->v = (s2)->v;}

/* Relative pointer to the stack */
typedef Tvalue *stkrel;

#define i2obj(i) ((Tvalue){.tt_=CYTH_INTEGER,.v.integer=(i)})
#define s2obj(s) ((Tvalue){.tt_=CYTH_STRING,.v.string=(s)})
#define t2obj(t) ((Tvalue){.tt_=CYTH_TABLE,.v.table=(t)})
#define f2obj(f) ((Tvalue){.tt_=CYTH_FUNCTION,.v.function=(f)})
#define ud2obj(ud) ((Tvalue){.tt_=CYTH_USERDATA,.v.userdata=(ud)})
#define b2obj(b) ((Tvalue){.tt_=CYTH_BOOL,.v.boolean=(b)})
#define a2obj(a) ((Tvalue){.tt_=CYTH_ARRAY,.v.array=(a)})

typedef int(*cyth_Pfunction)(cyth_State*, void*);

typedef struct {
  char *data;
  cmem_t n;
  cmem_t s;
} SBuffer;

Table *cythH_new(cyth_State *C);
int cythH_append(cyth_State *C, Table *t, Tvalue i, Tvalue v);
void cythH_get(cyth_State *C, Table *t, Tvalue i, Tvalue *res);
int cythH_remove(cyth_State *C, Table *t, Tvalue i);
void cythH_free(cyth_State *C, Table *t);

Array *cythR_new(cyth_State *C);
void cythR_push(cyth_State *C, Array *a, cyth_integer k, Tvalue v);
void cythR_get(cyth_State *C, Array *a, cyth_integer k, Tvalue *res);
void cythR_remove(cyth_State *C, Array *a, cyth_integer k);
void cythR_free(cyth_State *C, Array *a);

void cythO_buffer_new(SBuffer *s);
void cythO_buffer_appendstr(cyth_State *C, SBuffer *s, char *str, cmem_t len);
void cythO_buffer_appendchar(cyth_State *C, SBuffer *s, char c);
void cythO_buffer_rewind(cyth_State *C, SBuffer *s);
void cythO_buffer_free(cyth_State *C, SBuffer *s);
#endif
