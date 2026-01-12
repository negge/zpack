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

void br_init(bit_reader *const br, const unsigned char *const in, int size) {
  br->in = in;
  br->size = size;
  br->mask = br->idx = br->bits = 0;
}

unsigned char read_byte(bit_reader *const br) {
  ZPACK_ERROR(br->idx > br->size, ("Read past end of input buffer"));
  br->bits += 8;
  return br->in[br->idx++];
}

int read_bit(bit_reader *const br) {
  br->mask >>= 1;
  if (!br->mask) {
    br->mask = 128;
    br->val = read_byte(br);
    br->bits -= 8;
  }
  br->bits++;
  return br->val & br->mask ? 1 : 0;
}

int read_length(bit_reader *const br) {
  int len = 1;
  while (read_bit(br)) {
    len <<= 1;
    len |= read_bit(br);
  }
  return len;
}
