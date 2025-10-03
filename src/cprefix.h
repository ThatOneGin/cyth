#ifndef CPREFIX_H
#define CPREFIX_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>

#ifndef cyth_assert
#include <assert.h>
#define cyth_assert(e) assert(e)
#endif

/* error macro to use when no cyth_State is available */
#ifndef cyth_rawerr
#define cyth_rawerr(...) \
  (fprintf(stderr, "[Error]: "), fprintf(stderr, __VA_ARGS__), exit(1))
#endif

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
