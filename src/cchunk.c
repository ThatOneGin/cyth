#include <cchunk.h>
#include <caux.h>
#include <cmem.h>
#include <cstring.h>
#include <string.h>


#define DEFAULTINTSIZE (sizeof(cyth_integer))

/* unloader */

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
  case CYTH_BOOL:
    unload_byte(U, obj2b(v));
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

/* loader */

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
  case CYTH_BOOL:
    load_byte(L, (byte*)&obj2b(v));
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

/* printer */

#define Putc(c) putc(c, stdout)

static void print_int(cyth_integer i) {
  printf("%ld", i);
}

static void print_string(String *s) {
  Putc('"');
  char *p = s->data;
  for (cmem_t i = 0; i < s->len; p++, i++) {
  switch (*p) {
    case '"': printf("\\\""); break;
    case '\a': printf("\\a"); break;
    case '\b': printf("\\b"); break;
    case '\f': printf("\\f"); break;
    case '\n': printf("\\n"); break;
    case '\r': printf("\\r"); break;
    case '\t': printf("\\t"); break;
    case '\v': printf("\\v"); break;
    default: Putc(*p); break;
  }
 }
  Putc('"');
}

static void print_value(Tvalue v) {
  switch (cyth_tt(&v)) {
  case CYTH_INTEGER:
    print_int(obj2i(&v));
    break;
  case CYTH_STRING:
    print_string(obj2s(&v));
    break;
  case CYTH_NONE:
    printf("none");
    break;
  case CYTH_BOOL:
    printf("%s", obj2b(&v) ? "true" : "false");
    break;
  default: /* should never happen */
    printf("?");
    break;
  }
}

static void print_constants(cyth_Function *f) {
  for (cmem_t i = 0; i < f->nk; i++) {
    printf("\t%lu\t", i);
    print_value(f->k[i]);
    Putc('\n');
  }
}

static byte print_az_as_value[OP_COUNT] = {
  [OP_PUSH] = 1,
  [OP_POP] = 0,
  [OP_ADD] = 0,
  [OP_SETVAR] = 1,
  [OP_GETVAR] = 1,
  [OP_EQ] = 0,
  [OP_NEQ] = 0,
  [OP_JMP] = 0,
};

static void print_code(cyth_Function *f) {
  int opcode;
  int line;
  argZ az;
  Instruction j;
  cyth_assert(f->ncode == f->nline);
  for (cmem_t i = 0; i < f->ncode; i++) {
    Putc('\t');
    j = f->code[i];
    opcode = getopcode(j);
    az = getargz(j);
    line = f->lineinfo[i];
    printf("%lu\t", i);
    printf("[");
    if (line > 0) printf("%d", line);
    else printf("#");
    printf("]\t");
    printf("%s\t%d\t", cythC_getopcode(opcode), az);
    if (opcode < OP_COUNT && print_az_as_value[opcode]) {
      printf("; ");
      print_value(f->k[az]);
      Putc('\n');
    } else {
      Putc('\n');
    }
  }
}

void cythL_print(cyth_Function *f) {
  printf("Function %p (%lu):\n", (void*)f, f->ncode);
  print_code(f);
  printf("constants for %p (%lu):\n", (void*)f, f->nk);
  print_constants(f);
}
