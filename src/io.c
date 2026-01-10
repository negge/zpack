#include <stdio.h>
#include <string.h>
#include "internal.h"
#include "io.h"

#define ZPACK_FEOF(fp) (ungetc(getc(fp), fp) == EOF)

int load(char *const input, unsigned char *const in, const int len) {
  FILE *fp;
  int s;
  fp = stdin;
  if (strcmp(input, "-") != 0) {
    fp = fopen(input, "r");
    ZPACK_ERROR(fp == NULL, ("Could not open file %s", input));
  }
  for (s = 0; !ZPACK_FEOF(fp); s++) {
    ZPACK_ERROR(s == len, ("Max input file size %i exceeded!", len));
    in[s] = getc(fp);
  }
  fclose(fp);
  return s;
}

void store(char *const output, const unsigned char *const out, const int len) {
  FILE *fp;
  fp = stdout;
  if (strcmp(output, "-") != 0) {
    fp = fopen(output, "wb");
    ZPACK_ERROR(fp == NULL, ("Unable to open output file '%s'", output));
  }
  fwrite(out, len, 1, fp);
  fclose(fp);
}
