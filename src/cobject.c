#include <cobject.h>
#include <cmem.h>
#include <cgc.h>
#include <cvm.h>

List *cythO_listnew(cyth_State *C) {
  gc_object *ref = cythG_newobj(C, GCOL);
  ref->v.l = cythM_malloc(C, sizeof(List));
  List *l = ref->v.l;
  l->items = NULL;
  l->itemsize = 0;
  l->nitems = 0;
  return l;
}

int cythO_listappend(cyth_State *C, List *l, Tvalue v) {
  if (l->nitems >= l->itemsize)
    cythM_vecgrow(C, l->items, l->itemsize, Tvalue);
  l->items[l->nitems++] = v;
  return l->nitems-1;
}

void cythO_listfree(cyth_State *C, List *l) {
  cythM_vecfree(C, l->items, l->itemsize, Tvalue);
  l->nitems = 0;
}

Table *cythH_new(cyth_State *C) {
  gc_object *ref = cythG_newobj(C, GCOT);
  ref->v.l = cythM_malloc(C, sizeof(Table));
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
