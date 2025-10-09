#ifndef CCHUNK_H
#define CCHUNK_H
#include <cio.h>

/* A chunk is just a function. */

#define CYTH_MAGIC "\001CYTH"

typedef int(*cyth_Writer)(cyth_State *, void *, size_t, void *);

/*
** In this context, unload means to discharge
** a function from memory to a file and 'load'
** means the opposite (file to memory).
**
** When loading/unloading a function, the following order
** should be used:
**  - f->ncode (2 bytes)
**  - f->nk (2 bytes)
**  - f->nline (2 bytes)
**  - f->codesize (2 bytes)
**  - f->ksize (2 bytes)
**  - f->linesize (2 bytes)
**  - f->code (vector)
**  - f->lineinfo (vector)
**  - f->k (vector)
*/

void cythU_unload(cyth_State *C, cyth_Function *f, cyth_Writer u, void *aux);
void cythL_load(cyth_State *C, Stream *input, char *name);
void cythL_print(cyth_Function *f);
#endif