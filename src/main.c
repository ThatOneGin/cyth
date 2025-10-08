#include <cstate.h>
#include <cstring.h>
#include <cparser.h>
#include <caux.h>
#include <cchunk.h>

static int generic_writer(cyth_State *C, void *b, size_t size, void *aux) {
  (void)C;
  return fwrite(b, size, 1, (FILE*)aux);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  (void)generic_writer;
  cyth_State *C = cythE_openstate();
  if (argc < 2)
    cythE_error(C, "No arguments provided.");
#if 0 /* compile and unload */
  cythI_loadfile(C, argv[1]);
  FILE *d = fopen("a.out", "wb");
  if (d == NULL) goto defer;
  Tvalue f = cythA_pop(C);
  cythU_unload(C, obj2f(&f), generic_writer, d);
defer:
  if (d != NULL) fclose(d);
#elif 0 /* load and execute */
  cythI_loadfile(C, argv[1]);
  cythF_call(C, -1, 0);
  printf("%d\n", cythA_popint(C));
#endif
  cythE_closestate(C);
  return 0;
}