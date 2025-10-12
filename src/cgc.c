#include <cgc.h>
#include <cmem.h>
#include <cobject.h>

/*
** This garbage collector should be incremental,
** but we want to maintain simplicity for now
** so it stays like a to-do-later thing.
*/

/* get raw pointers of values */

static void *val2ptr(Tvalue v) {
  switch (cyth_tt(&v)) {
  case CYTH_STRING: return obj2s(&v)->data;
  case CYTH_TABLE: return obj2t(&v)->list;
  case CYTH_FUNCTION: return obj2f(&v);
  default: return NULL;
  }
}

static void *gco2ptr(gc_object *o) {
  switch (cyth_tt(o)) {
  case GCOS: return o->v.s->data;
  case GCOT: return o->v.t->list;
  case GCOF: return o->v.f;
  default: return NULL;
  }
}

static void freeobj(cyth_State *C, gc_object *o) {
  switch (o->tt_) {
  case GCOS:
    cythM_free(C, o->v.s->data, o->v.s->len);
    cythM_free(C, o->v.s, sizeof(o->v.s));
    break;
  case GCOT:
    cythH_free(C, o->v.t);
    cythM_free(C, o->v.t, sizeof(o->v.t));
    break;
  case GCOF:
    cythF_freefunc(o->v.f);
    break;
  }
  cythM_free(C, o, sizeof(gc_object));
}

/* mark a tagged value */
static void markvalue(global_State *G, Tvalue v) {
  gc_object *l = G->list;
  void *vptr = val2ptr(v);
  if (vptr == NULL) return; /* avoid work */
  while (l != NULL) {
    void *optr = gco2ptr(l);
    if (optr == vptr)
      l->mark = 1;
    l = l->next;
  }
  if (cyth_tt(&v) == CYTH_TABLE) {
    Table *t = obj2t(&v);
    Node *l = t->list;
    while (l != NULL) {
      markvalue(G, l->key);
      markvalue(G, l->val);
      l = l->next;
    }
  } else if (cyth_tt(&v) == CYTH_FUNCTION) {
    cyth_Function *f = obj2f(&v);
    for (cmem_t i = 0; i < f->nk; i++)
      markvalue(G, f->k[i]);
  }
}

/*
** As the only place we can look for
** objects is the stack, we gonna go
** look for some.
*/
static void markphase(cyth_State *C) {
  for (stkrel v = C->base; v != C->top; v++) {
    markvalue(C->G, *v);
  }
}

/*
** the sweep phase
**  unmark the reachable objects
**  and free the unreachable ones
*/
static void sweeplist(cyth_State *C, gc_object **l, size_t count) {
  while (*l != NULL && count > 0) {
    gc_object *curr = *l;
    if (!curr->mark) {
      *l = curr->next;
      freeobj(C, curr);
      count--;
    } else {
      curr->mark = 0;
      l = &curr->next;
    }
  }
}

/* perform a full cycle */
void cythG_full(cyth_State *C) {
  global_State *G = C->G;
  if (G->count >= GCTHRESHOLD) {
    markphase(C);
    sweeplist(C, &G->list, GCMAXSWEEP);
  }
}

cmem_t cythG_total(cyth_State *C) {
  return C->G->total;
}

cmem_t cythG_inuse(cyth_State *C) {
  return C->G->count;
}

static void deletelist(cyth_State *C, gc_object *l) {
  while (l != NULL) {
    gc_object *next= l->next;
    freeobj(C, l);
    l = next;
  }
}

/* 
** when the main cyth_State dies or is closed,
** this should be called to free all memory used
** by the program
*/
void cythG_freeall(cyth_State *C) {
  global_State *G = C->G;
  deletelist(C, G->list);
  deletelist(C, G->uncollectables);
}

/* allocate a normal gc object */
gc_object *cythG_newobj(cyth_State *C, byte tt_) {
  global_State *G = C->G;
  gc_object *o = cythM_malloc(C, sizeof(gc_object));
  o->tt_ = tt_;
  o->mark = 0;
  /* put it in the list */
  o->next = G->list;
  G->list = o;
  return o;
}

/*
** Makes an object uncollectable
** object must be the last in GC list.
*/
void cythG_uncoll(cyth_State *C, gc_object *o) {
  global_State *G = C->G;
  cyth_assert(G->list == o); /* must be the last element */
  G->list = G->list->next;
  o->next = G->uncollectables;
  G->uncollectables = o;
}
