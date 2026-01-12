#include <stdio.h>
#include "internal.h"

static int m, s, i, o, v, size, bits;
static const unsigned char *in;

static unsigned char read_byte() {
  ZPACK_ERROR(s > size, ("Read past end of input buffer"));
  bits += 8;
  return in[s++];
}

static int read_bit() {
  m >>= 1;
  if (!m) {
    m = 128;
    v = read_byte();
    bits -= 8;
  }
  bits++;
  return v & m ? 1 : 0;
}

static int read_length() {
  int len = 1;
  while (read_bit()) {
    len <<= 1;
    len |= read_bit();
  }
  return len;
}

static void write_byte(unsigned char *const out, unsigned char byte) {
  ZPACK_ERROR(i > DATA_MAX, ("Error, decoded payload exceeds %i", DATA_MAX));
  out[i++] = byte;
}

static int unpack(unsigned char *const out) {
  m = s = i = bits = 0;
  o = 1;
  while (1) {
    int n, len = read_length();
    while (len-- > 0) write_byte(out, read_byte());
    n = read_bit();
    do {
      if (n) {
        o = 255 - read_byte();
        if (!o) {
          ZPACK_ERROR(v & (m - 1), ("Trailing bits not zero %i", v));
          return bits;
        }
      }
      len = read_length();
      while (len-- > 0) write_byte(out, out[i - o]);
    }
    while ((n = read_bit()));
  }
}

void decompress(unsigned char *const out, const unsigned char *const input,
 const int sz, zpack_stats *stats) {
  in = input;
  size = sz;
  stats->bits = unpack(out);
  ZPACK_ERROR(s != size, ("Did not read all input %i %i", s, size));
  stats->size = i;
  stats->packed = sz;
}
