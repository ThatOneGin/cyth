#include <cio.h>
#include <cstring.h>
#include <caux.h>
#include <string.h>
#include <errno.h>
#include <cchunk.h>

struct BufferF {
  char data[IOBUFSIZE];
  FILE *f;
};

struct BufferS {
  char *data;
  size_t size;
};

/* reader functions */

/* read from a file buffer */
static char *readF(cyth_State *C, void *aux, size_t *size) {
  (void)C;
  struct BufferF *b = (struct BufferF*)aux;
  size_t nread = fread(b->data, sizeof(char), IOBUFSIZE, b->f);
  if (nread == 0) {
    if (ferror(b->f))
      return NULL; /* read error */
    else { /* end of file */
      *size = 0;
      return NULL;
    }
  }
  *size = nread;
  return b->data;
}

/* read from a string buffer */
static char *readS(cyth_State *C, void *aux, size_t *size) {
  (void)C;
  struct BufferS *b = (struct BufferS*)aux;
  if (b->size == 0) {
    *size = 0;
    return NULL;
  } else {
    *size = b->size;
    b->size = 0;
    return b->data;
  }
}

struct Stream {
  void *aux; /* buffer */
  char *curr; /* current char */
  cmem_t size; /* how many to read */
  cyth_State *C;
  cyth_Reader read;
};

void cythI_new(cyth_State *C, Stream *s, cyth_Reader read, void *aux) {
  s->size = 0;
  s->aux = aux;
  s->curr = NULL;
  s->C = C;
  s->read = read;
  s->size = 0;
}

static int fill_buffer(Stream *s) {
  if (s->size > 0) {
    return 0; /* buffer still has something */
  } else { /* it is empty */
    size_t size;
    char *buffer;
    buffer = s->read(s->C, s->aux, &size);
    if (buffer == NULL)
      return EOS;
    s->curr = buffer;
    s->size = size;
    return 1;
  }
}

int cythI_getc(Stream *s) {
  int status = fill_buffer(s);
  if (status == EOS) return EOS;
  else {
    if (s->size == 0)
      return EOS;
    int c = *s->curr;
    s->curr++;
    s->size--;
    return c;
  }
}

void cythI_ungetc(Stream *s) {
  s->curr--;
  s->size++;
}

int cythI_read(Stream *s, void *b, cmem_t n) {
  size_t o_n;
  if (fill_buffer(s) == EOS) {
    return 1;
  }
  o_n = (n <= s->size) ? n : s->size;
  memcpy(b, s->curr, o_n);
  s->size -= o_n;
  s->curr += o_n;
  return 0;
}

static void file_error(cyth_State *C, const char *msg,  const char *filename) {
  cythE_error(C, "%s %s: %s", msg, filename, strerror(errno));
}

/* compile file */
void cythI_loadfile(cyth_State *C, char *filename) {
  struct BufferF b;
  Stream s;
  b.f = fopen(filename, "r");
  if (b.f == NULL)
    file_error(C, "Couldn't open", filename);
  cythI_new(C, &s, readF, &b);
  if (cythI_getc(&s) == CYTH_MAGIC[0]) {
    fclose(b.f);
    b.f = fopen(filename, "rb"); /* reopen as 'rb' */
    if (b.f == NULL)
      file_error(C, "Couldn't open", filename);
    cythI_new(C, &s, readF, &b);
    cythL_load(C, &s, filename);
    fclose(b.f);
    return;
  } else /* else put char back */
    cythI_ungetc(&s);
  if (cythA_load(C, &s, filename)) { /* failed */
    fclose(b.f);
    cythE_throw(C, 1, cythA_popstr(C));
  } else {
    fclose(b.f);
  }
}

/* compile string (for tests) */
void cythI_loadstring(cyth_State *C, char *chunkname, char *chunk) {
  struct BufferS b;
  b.data = chunk;
  b.size = strlen(b.data);
  Stream s;
  cythI_new(C, &s, readS, &b);
  if (cythA_load(C, &s, chunkname))
    cythE_throw(C, 1, cythA_popstr(C));
}
