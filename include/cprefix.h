#ifndef CPREFIX_H
#define CPREFIX_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <stddef.h>

#ifndef cyth_assert
#include <assert.h>
#define cyth_assert(e) assert(e)
#endif

#ifndef cyth_write
#define cyth_write(s, stream) fputs(s, stream)
#endif

#define cyth_writestring(s) cyth_write(s, stdout)
#define cyth_writeerror(s) \
  (cyth_write("[Error]: ", stderr), (cyth_write(s, stderr)))

typedef int8_t sbyte;
typedef uint8_t byte;
typedef int64_t cyth_integer;
typedef size_t cmem_t;

#define cyth_try(C, f, ud) \
  if (setjmp((C)->errhandler->buf) == 0) \
    f(C, ud);
#define cyth_throw(C, c) \
  if ((C)->errhandler != NULL) \
    longjmp((C)->errhandler->buf, (c));
#endif
