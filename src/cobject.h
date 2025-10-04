#ifndef COBJECT_H
#define COBJECT_H
#include <cprefix.h>

typedef struct cyth_State cyth_State;
typedef struct global_State global_State;

typedef struct {
  char *data;
  size_t len;
} String;

/*
** We have 2 kinds of lists:
** a 'List' is an array of values, and a 'Table' which is an
** associative array, that is, every value can be used as keys.
**
** A list should be used to describe a S-expression content
** in a flat way.
*/
typedef struct List List;
typedef struct Table Table;

#define CYTH_NONE 0
#define CYTH_INTEGER 1
#define CYTH_STRING 2
#define CYTH_LITERAL 3
#define CYTH_LIST 4
#define CYTH_TABLE 5
#define CYTH_FUNCTION 6
#define CYTH_USERDATA 7

#define NONE ((Tvalue){.tt_=CYTH_NONE,{0}})

typedef struct cyth_Function cyth_Function;

/* types of user data */
#define UDFUN 0
#define UDVAL 1

typedef struct {
  byte type;
  union {
    int (*cfunc)(cyth_State*);
    void *val;
  } u;
} userdata;

typedef union {
  cyth_integer integer;
  String *string;
  String *literal;
  List *list;
  Table *table;
  cyth_Function *function;
  userdata userdata;
} Value;

typedef struct {
  byte tt_;
  Value v;
} Tvalue;

struct List {
  Tvalue *items;
  cmem_t nitems;
  cmem_t itemsize;
};

typedef struct Node {
  Tvalue key;
  Tvalue val;
  struct Node *next;
} Node;

struct Table {
  Node *list;
  int len;
};

#define cyth_tt(o) ((o)->tt_)
#define obj2i(o) (o)->v.integer
#define obj2s(o) (o)->v.string
#define obj2l(o) (o)->v.literal
#define obj2ls(o) (o)->v.list
#define obj2t(o) (o)->v.table
#define obj2f(o) (o)->v.function
#define obj2ud(o) (o)->v.userdata
#define objcopy(s1, s2) {\
  cyth_tt(s1) = cyth_tt(s2); \
  (s1)->v = (s2)->v}

/* Relative pointer to the stack */
typedef Tvalue *stkrel;

#define i2obj(i) ((Tvalue){.tt_=CYTH_INTEGER,.v.integer=(i)})
#define s2obj(s) ((Tvalue){.tt_=CYTH_LITERAL,.v.string=(s)})
#define l2obj(l) ((Tvalue){.tt_=CYTH_LITERAL,.v.literal=(l)})
#define ls2obj(l) ((Tvalue){.tt_=CYTH_LIST,.v.list=(l)})
#define t2obj(t) ((Tvalue){.tt_=CYTH_TABLE,.v.table=(l)})
#define f2obj(f) ((Tvalue){.tt_=CYTH_FUNCTION,.v.function=(f)})
#define ud2obj(ud) ((Tvalue){.tt_=CYTH_USERDATA,.v.userdata=(ud)})

typedef int(*cyth_Pfunction)(cyth_State*, void*);

List *cythO_listnew(cyth_State *C);
int cythO_listappend(cyth_State *C, List *l, Tvalue v);
void cythO_listfree(cyth_State *C, List *l);
Table *cythH_new(cyth_State *C);
int cythH_append(cyth_State *C, Table *t, Tvalue i, Tvalue v);
void cythH_get(cyth_State *C, Table *t, Tvalue i, Tvalue *res);
int cythH_remove(cyth_State *C, Table *t, Tvalue i);
void cythH_free(cyth_State *C, Table *t);
#endif
