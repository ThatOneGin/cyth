#include <cchunk.h>
#include <caux.h>
#include <cmem.h>
#include <cstring.h>
#include <string.h>

#define DEFAULTINTSIZE (sizeof(cyth_integer))
#define unload_vector(U, v, n) unload_bytearray(U, v, (sizeof(*v) * (n)))

typedef struct {
  cyth_State *C;
  cyth_Writer writer;
  void *aux; /* file */
  size_t offset;
} Unloader; /* Unloader state */

static void unload_bytearray(Unloader *U, void *b, size_t s) {
  U->writer(U->C, b, s, U->aux);
  U->offset += s;
}

static void unload_byte(Unloader *U, int b) {
  byte c = (byte)b;
  unload_bytearray(U, &c, sizeof(c));
}

static void unload_int(Unloader *U, cyth_integer i, int size) {
  if ((size % 2) != 0)
    size += 1;
  unload_bytearray(U, (char*)(&i), size);
}

static void unload_string(Unloader *U, String *s) {
  unload_int(U, s->len, DEFAULTINTSIZE);
  unload_bytearray(U, s->data, s->len);
}

static void unload_value(Unloader *U, Tvalue *v) {
  unload_byte(U, cyth_tt(v));
  switch (cyth_tt(v)) {
  case CYTH_NONE:
    unload_byte(U, 0);
    break;
  case CYTH_INTEGER:
    unload_int(U, obj2i(v), DEFAULTINTSIZE);
    break;
  case CYTH_STRING:
    unload_string(U, obj2s(v));
    break;
  default: 
    cyth_assert(0); /* unreachable */
  }
}

static void unload_constants(Unloader *U, cyth_Function *f) {
  for (cmem_t i = 0; i < f->nk; i++) {
    unload_value(U, &f->k[i]);
  }
}

static void unload_function(Unloader *U, cyth_Function *f) {
  unload_int(U, f->ncode, sizeof(int16_t));
  unload_int(U, f->nk, sizeof(int16_t));
  unload_int(U, f->nline, sizeof(int16_t));
  unload_int(U, f->codesize, sizeof(int16_t));
  unload_int(U, f->ksize, sizeof(int16_t));
  unload_int(U, f->linesize, sizeof(int16_t));
  unload_vector(U, f->code, f->ncode);
  unload_vector(U, f->lineinfo, f->nline);
  unload_constants(U, f);
}

void unload_header(Unloader *U) {
  unload_bytearray(U, CYTH_MAGIC, sizeof(CYTH_MAGIC) - 1);
}

/* Unload function from memory to file. */
void cythU_unload(cyth_State *C, cyth_Function *f, cyth_Writer u, void *aux) {
  Unloader U;
  U.C = C;
  U.offset = 0;
  U.writer = u;
  U.aux = aux;
  unload_header(&U);
  unload_function(&U, f);
}

#define load_vector(L, v, n, t) (load_bytearray(L, v, sizeof(t)*(n)))

typedef struct {
  char *name;
  Stream *input; /* input stream */
  cyth_State *C;
  size_t offset;
} Loader;

static void unexpected_EOS(Loader *L) {
  cythE_error(L->C, "Unexpected EOS at %s (offset=%lu).", L->name, L->offset);
}

static void load_bytearray(Loader *L, void *b, size_t s) {
  if (cythI_read(L->input, b, s)) {
    unexpected_EOS(L);
  } else {
    L->offset += s;
  }
}

static void load_byte(Loader *L, byte *b) {
  load_bytearray(L, b, sizeof(*b));
}

static void load_int(Loader *L, cyth_integer *i, int size) {
  load_bytearray(L, i, size);
}

static void load_size(Loader *L, cmem_t *s, int size) {
  load_bytearray(L, s, size);
}

static void load_string(Loader *L, String **s) {
  cmem_t len;
  load_size(L, &len, DEFAULTINTSIZE);
  *s = cythS_newstrobj(L->C, len);
  load_bytearray(L, (*s)->data, len);
  (*s)->len = len;
  cythS_finishstrobj(L->C, s);
}

static void load_value(Loader *L, Tvalue *v) {
  load_byte(L, &v->tt_);
  switch (cyth_tt(v)) {
  case CYTH_NONE: {
    load_byte(L, (byte*)&v->v.integer);
    break;
  } case CYTH_INTEGER:
    load_int(L, &obj2i(v), DEFAULTINTSIZE);
    break;
  case CYTH_STRING:
    load_string(L, &obj2s(v));
    break;
  default: 
    cyth_assert(0); /* unreachable */
  }
}

static void load_constants(Loader *L, cyth_Function *f) {
  for (cmem_t i = 0; i < f->nk; i++) {
    load_value(L, &f->k[i]);
  }
}

static void load_header(Loader *L) {
  char magic[6] = {0};
  load_bytearray(L, magic, 5);
  if (strcmp(magic, CYTH_MAGIC) != 0) {
    cythE_error(L->C, "Corrupted file.\n");
  }
}

/* load a bytecode file */
void cythL_load(cyth_State *C, Stream *input, char *name) {
  Loader L;
  L.C = C;
  L.name = (name != NULL) ? name : "binary chunk";
  L.input = input;
  L.offset = 0;
  cyth_Function *f = cythF_newfunc(C);
  load_header(&L);
  load_size(&L, &f->ncode, sizeof(int16_t));
  load_size(&L, &f->nk, sizeof(int16_t));
  load_size(&L, &f->nline, sizeof(int16_t));
  load_size(&L, &f->codesize, sizeof(int16_t));
  load_size(&L, &f->ksize, sizeof(int16_t));
  load_size(&L, &f->linesize, sizeof(int16_t));
  f->codesize = f->ncode;
  f->ksize = f->nk;
  f->linesize = f->nline;
  f->code = cythM_malloc(C, sizeof(*f->code) * f->ncode);
  f->lineinfo = cythM_malloc(C, sizeof(*f->lineinfo) * f->nline);
  f->k = cythM_malloc(C, sizeof(*f->k) * f->nk);
  load_vector(&L, f->code, f->ncode, Instruction);
  load_vector(&L, f->lineinfo, f->nline, int);
  load_constants(&L, f);
  cythA_push(C, f2obj(f));
}
