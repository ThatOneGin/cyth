#ifndef CIO_H
#define CIO_H
#include <cstate.h>

#define EOS (-1) /* end of stream */
#define IOBUFSIZE BUFSIZ /* the same as stdio.h one */

/* a stream can have its source on a file or in a string */
typedef struct Stream Stream;

/*
** We need to have this function as a field in Stream
** because the stream can reference both strings and files
** so this makes easier to read without worrying about where
** the buffer will get data.
*/
typedef char *(*cyth_Reader)(cyth_State *, void *, size_t *);

void cythI_new(cyth_State *C, Stream *s, cyth_Reader read, void *aux);
int cythI_getc(Stream *s);
int cythI_read(Stream *s, void *b, cmem_t n);
void cythI_close(Stream *s);
void cythI_loadfile(cyth_State *C, char *filename);
void cythI_loadstring(cyth_State *C, char *chunkname, char *chunk);
#endif