#ifndef COBJECT_H
#define COBJECT_H
#include <cprefix.h>

typedef struct cyth_State cyth_State;
typedef struct global_State global_State;

typedef struct {
  char *data;
  size_t len;
} String;

#define CYTH_NONE 0
#define CYTH_INTEGER 1
#define CYTH_STRING 2
#define CYTH_LITERAL 3

typedef union {
  cyth_integer integer;
  String *string;
  String *literal;
} Value;

typedef struct {
  byte tt_;
  Value v;
} Tvalue;

#define cyth_tt(o) ((o)->tt_)
#define obj2i(o) (o)->v.integer
#define obj2s(o) (o)->v.string
#define obj2l(o) (o)->v.literal
#define objcopy(s1, s2) {\
  cyth_tt(s1) = cyth_tt(s2); \
  (s1)->v = (s2)->v}

/* Relative pointer to the stack */
typedef Tvalue *stkrel;

#define i2obj(i) ((Tvalue){.tt_=CYTH_INTEGER,.v.integer=(i)})
#define s2obj(s) ((Tvalue){.tt_=CYTH_LITERAL,.v.string=(s)})
#define l2obj(l) ((Tvalue){.tt_=CYTH_LITERAL,.v.literal=(l)})

typedef int(*cyth_Pfunction)(cyth_State*, void*);
#endif
