#include <cobject.h>
#include <cmem.h>
#include <cgc.h>
#include <cvm.h>

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
