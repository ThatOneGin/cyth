#include <cobject.h>
#include <cmem.h>
#include <cgc.h>
#include <cvm.h>
#include <string.h>

Table *cythH_new(cyth_State *C) {
  gc_object *ref = cythG_newobj(C, GCOT);
  ref->v.t = cythM_malloc(C, sizeof(Table));
  Table *t = ref->v.t;
  t->list = NULL;
  t->len = 0;
  return t;
}

static void node_new(cyth_State *C, Table *t) {
  Node *n = cythM_malloc(C, sizeof(Node));
  n->key = NONE;
  n->val = NONE;
  n->next = t->list;
  t->list = n;
}

int cythH_append(cyth_State *C, Table *t,
                 Tvalue i, Tvalue v) {
  Node *l = t->list;
  while (l != NULL && cyth_tt(&l->key) != CYTH_NONE) {
    if (cythV_objequ(C, l->key, i))
      break;
    l = l->next;
  }
  if (l == NULL) {
    node_new(C, t);
    t->list->key = i;
    t->list->val = v;
    t->len++;
  } else {
    l->key = i;
    l->val = v;
  }
  return t->len;
}

void cythH_get(cyth_State *C, Table *t, Tvalue i, Tvalue *res) {
  Node *l = t->list;
  while (l != NULL) {
    if (cythV_objequ(C, l->key, i))
      break;
    l = l->next;
  }
  *res = (l == NULL) ? NONE : l->val;
}

int cythH_remove(cyth_State *C,
                 Table *t, Tvalue i) {
  Node *l = t->list;
  while (l != NULL) {
    if (cythV_objequ(C, l->key, i))
      break;
    l = l->next;
  }
  if (l != NULL) {
    l->val = NONE;
    return 0;
  }
  return -1;
}

void cythH_free(cyth_State *C, Table *t) {
  Node *l = t->list;
  while (l != NULL) {
    Node *next = l->next;
    cythM_free(C, l, sizeof(*l));
    l = next;
  }
  t->list = NULL;
  t->len = 0;
}

Array *cythR_new(cyth_State *C) {
  gc_object *ref = cythG_newobj(C, GCOA);
  Array *a = cythM_malloc(C, sizeof(Array));
  a->arraysize = 2;
  a->narray = 0;
  cythM_vecnew(C, a->data, a->arraysize, Tvalue);
  ref->v.a = a;
  return a;
}

void cythR_push(cyth_State *C, Array *a,
               cyth_integer k, Tvalue v) {
  if (k < 0 || (cmem_t)k > a->narray)
    cythE_error(C, "index %ld is out of range", k);
  if (a->narray >= a->arraysize)
    cythM_vecgrow(C, a->data, a->arraysize, Tvalue);
  a->data[k] = v;
  if ((cmem_t)k == a->narray)
    a->narray++;
}

void cythR_get(cyth_State *C, Array *a, cyth_integer k, Tvalue *res) {
  if (k < 0 || (cmem_t)k > a->narray)
    cythE_error(C, "index %ld is out of range", k);
  *res = a->data[k];
}

void cythR_remove(cyth_State *C, Array *a, cyth_integer k) {
  if (k < 0 || (cmem_t)k > a->narray)
    cythE_error(C, "index %ld is out of range", k);
  for (cmem_t i = (cmem_t)k; i < a->narray - 1; i++) {
    a->data[i] = a->data[i+1];
  }
  a->narray--;
}

void cythR_free(cyth_State *C, Array *a) {
  cythM_vecfree(C, a->data, a->arraysize, Tvalue);
  a->narray = 0;
}

void cythO_buffer_new(SBuffer *s) {
  s->data = NULL;
  s->n = 0;
  s->s = 0;
}

void cythO_buffer_appendstr(cyth_State *C, SBuffer *s,
                            char *str, cmem_t len) {
  cmem_t toappend = len == 0 ? strlen(str) : len;
  if (toappend == 0) return;
  if (s->n >= s->s)
    cythM_vecgrow(C, s->data, s->s, char);
  memcpy(s->data+s->n, str, toappend);
  s->n += toappend;
}

void cythO_buffer_rewind(cyth_State *C, SBuffer *s) {
  (void)C;
  s->n = 0;
  if (s->data) s->data[0] = '\0';
}

void cythO_buffer_free(cyth_State *C, SBuffer *s) {
  cythM_vecfree(C, s->data, s->s, char);
}

void cythO_buffer_appendchar(cyth_State *C,
                             SBuffer *s, char c) {
  cythO_buffer_appendstr(C, s, &c, 1);
}
