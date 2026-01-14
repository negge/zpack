#include <stdio.h>
#include <string.h>
#include "internal.h"
#include "io.h"

static void update_stats(zpack_stats *const stats, const block_t blk,
 const int len, const int bits) {
  stats->blocks++;
  stats->hist[blk].cnt++;
  stats->hist[blk].bits += 1 + bits + (blk == UPD)*8 + (blk == LIT ? len*8 : 0);
  stats->hist[blk].bytes += len;
}

static void unpack(bit_reader *const br, unsigned char *const out,
 zpack_stats *const stats, const int flags) {
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

  memset(&stats->hist, 0, sizeof(stats->hist));
  /* Adjust for the first literal block having implicit leading 0 */
  stats->hist[LIT].bits = -1;
  /* Include EOF block here */
  stats->blocks = 1;
  while (1) {
    int n, bits, len = read_length(br, &bits);
    update_stats(stats, LIT, len, bits);
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
      len = read_length(br, &bits) + !!(flags & MOD_EXT_CPY);
      update_stats(stats, n ? UPD : CPY, len, bits);
      while (len-- > 0) write_byte(out[i - o]);
    }
    while ((n = read_bit(br)));
  }
}

void decompress(unsigned char *const out, const unsigned char *const in,
 const int size, zpack_stats *stats, const int flags) {
  bit_reader br;
  br_init(&br, in, size);
  unpack(&br, out, stats, flags);
}
