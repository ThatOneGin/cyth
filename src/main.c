#include <cstate.h>
#include <cstring.h>
#include <cparser.h>
#include <caux.h>
#include <cchunk.h>
#include <string.h>

#define longargcmp(arg, opt) (strcmp(arg+2, opt)==0)

static byte load = 0;
static byte print = 0;
static byte compile = 0;
static char *prog = NULL;
static char *out = NULL;

static int handle_long_args(char *arg) {
  if (longargcmp(arg, "load")) {
    load = 1;
  } else if (longargcmp(arg, "print")) {
    print = 1;
  } else if (longargcmp(arg, "compile")) {
    compile = 1;
  } else {
    printf("Unrecognized option '%s'.\n", arg);
    return 1;
  }
  return 0;
}

static int handle_short_args(char **argv, int argc) {
  byte foundprog = 0;
  for (int i = 0; i < argc; i++) {
    if (argv[i][0] != '-' && !foundprog) {
      foundprog = 1;
      prog = argv[i];
    } else if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'c':
        compile = 1;
        break;
      case 'l':
        load = 1;
        break;
      case 'p':
        print = 1;
        break;
      case 'o':
        if (i+1 >= argc) {
          printf("Expected argument for '-o' option.\n");
          return 0;
        }
        out = argv[i+1];
        i++;
        break;
      case '-': /* long arg */
        if (handle_long_args(argv[i]))
          return 1;
        break;
      default:
        printf("Unrecognized short option '-%c'.\n", argv[i][1]);
        return 0;
      }
    }
  }
  return 1;
}

static int generic_writer(cyth_State *C, void *b, size_t size, void *aux) {
  (void)C;
  return fwrite(b, size, 1, (FILE*)aux);
}

static void help(void) {
  printf("Usage: cyth [OPTION] INPUT\n");
  printf("\t-l -- load: load bytecode file.\n");
  printf("\t-p -- print: print bytecode file.\n");
  printf("\t-c -- compile: compile source.\n");
}

int main(int argc, char **argv) {
  cyth_State *C = cythE_openstate();
  if (!handle_short_args(++argv, --argc))
    goto close;
  if (compile) {
    if (prog == NULL) cythE_error(C, "No input file.");
    cythI_loadfile(C, prog);
    FILE *d = fopen((out == NULL) ? "a.out" : out, "wb");
    if (d == NULL) goto defer;
    Tvalue f = cythA_pop(C);
    cythU_unload(C, obj2f(&f), generic_writer, d);
defer:
    if (d != NULL)
      fclose(d);
  } else if (load) {
    if (prog == NULL) cythE_error(C, "No input file.");
    cythI_loadfile(C, prog);
    cythF_call(C, -1, 0);
  } else if (print) {
    if (prog == NULL) cythE_error(C, "No input file.");
    cythI_loadfile(C, prog);
    Tvalue f = cythA_pop(C);
    cythL_print(obj2f(&f));
  } else
    help();
close:
  cythE_closestate(C);
  return 0;
}
