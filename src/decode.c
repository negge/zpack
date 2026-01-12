#include <stdio.h>
#include "internal.h"
#include "io.h"

static void unpack(bit_reader *const br, unsigned char *const out,
 zpack_stats *const stats) {
  /* Initial offset is 1 */
  int o = 1;
  int i = 0;

#define write_byte(b) \
  do { \
    ZPACK_ERROR(i > DATA_MAX, ("Decoded payload exceeds %i", DATA_MAX)); \
    out[i] = b; \
    i++; \
  } \
  while (0)

  while (1) {
    int n, len = read_length(br);
    while (len-- > 0) write_byte(read_byte(br));
    n = read_bit(br);
    do {
      if (n) {
        o = 255 - read_byte(br);
        /* offset 0 means EOF reached */
        if (!o) {
          ZPACK_ERROR(br->val & (br->mask - 1),
           ("Trailing bits not zero %i", br->val));
          ZPACK_ERROR(br->idx != br->size,
           ("Did not read all input %i != %i", br->idx, br->size));

          stats->size = i;
          stats->bits = br->bits;
          stats->packed = br->size;
          return;
        }
      }
      len = read_length(br);
      while (len-- > 0) write_byte(out[i - o]);
    }
    while ((n = read_bit(br)));
  }
}

void decompress(unsigned char *const out, const unsigned char *const in,
 const int size, zpack_stats *stats) {
  bit_reader br;
  br_init(&br, in, size);
  unpack(&br, out, stats);
}
