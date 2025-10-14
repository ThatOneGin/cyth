#ifndef COBJECT_H
#define COBJECT_H
#include <cprefix.h>

typedef struct cyth_State cyth_State;
typedef struct global_State global_State;

typedef struct {
  char *data;
  size_t len;
  /*
  ** This being set to UCHAR_MAX means
  ** that the content on data is not
  ** a reserved word
  */
  byte aux;
} String;

/*
** A 'Table' is an associative array,
** that is, every value can be used as keys.
*/
typedef struct Table Table;

#define CYTH_NONE 0
#define CYTH_INTEGER 1
#define CYTH_STRING 2
#define CYTH_LITERAL 3
#define CYTH_TABLE 4
#define CYTH_FUNCTION 5
#define CYTH_USERDATA 6
#define CYTH_BOOL 7

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
  Table *table;
  cyth_Function *function;
  userdata userdata;
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

#define cyth_tt(o) ((o)->tt_)
#define obj2i(o) (o)->v.integer
#define obj2s(o) (o)->v.string
#define obj2l(o) (o)->v.literal
#define obj2t(o) (o)->v.table
#define obj2f(o) (o)->v.function
#define obj2ud(o) (o)->v.userdata
#define obj2b(o) (o)->v.boolean
#define objcopy(s1, s2) {\
  cyth_tt(s1) = cyth_tt(s2); \
  (s1)->v = (s2)->v;}

/* Relative pointer to the stack */
typedef Tvalue *stkrel;

#define i2obj(i) ((Tvalue){.tt_=CYTH_INTEGER,.v.integer=(i)})
#define s2obj(s) ((Tvalue){.tt_=CYTH_STRING,.v.string=(s)})
#define l2obj(l) ((Tvalue){.tt_=CYTH_LITERAL,.v.literal=(l)})
#define t2obj(t) ((Tvalue){.tt_=CYTH_TABLE,.v.table=(t)})
#define f2obj(f) ((Tvalue){.tt_=CYTH_FUNCTION,.v.function=(f)})
#define ud2obj(ud) ((Tvalue){.tt_=CYTH_USERDATA,.v.userdata=(ud)})
#define b2obj(b) ((Tvalue){.tt_=CYTH_BOOL,.v.boolean=(b)})

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
void cythO_buffer_new(SBuffer *s);
void cythO_buffer_appendstr(cyth_State *C, SBuffer *s, char *str, cmem_t len);
void cythO_buffer_appendchar(cyth_State *C, SBuffer *s, char c);
void cythO_buffer_rewind(cyth_State *C, SBuffer *s);
void cythO_buffer_free(cyth_State *C, SBuffer *s);
#endif
