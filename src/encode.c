#include <stdio.h>
#include "internal.h"

#define MAX_OFFSET (255)

typedef struct cost_t cost_t;

struct cost_t {
  int bits;
  int len;
  int off;
  int next;
};

typedef struct entry_t entry_t;

struct entry_t {
  /* Initialized to input data */
  unsigned char byte;
  int last;

  /* best cost literal pass through, given offset */
  cost_t lit[MAX_OFFSET];

  /* best cost copy from offset, given offset */
  cost_t cpy[MAX_OFFSET];
};

static int elias_cost(int len) {
  int bits = 1;
  while (len >>= 1) bits += 2;
  return bits;
}

#define LIT_COST(len) (1 + 8*len + elias_cost(len))
#define CPY_COST(off, len) (1 + 8*(off) + elias_cost(len))

#define LARGE_COST (16*1024*8) /* Assume all packed files are < 16kB */
#define EOF_COST (1 + 8)

static void update(cost_t *c, int len, int off, int next, int bits) {
  if (bits < c->bits || (bits == c->bits && len < c->len)) {
    c->bits = bits;
    c->len = len;
    c->off = off;
    c->next = next;
  }
}

static int msb(int v) {
  int ret = 0;
  while (v >>= 1) ret++;
  return ret;
}

static int compute(entry_t *const tbl, const int size) {
  int p, o, i;
  /* Set initial costs to very large value */
  for (p = 0; p < size; p++) for (o = 0; o < MAX_OFFSET; o++) {
    tbl[p].lit[o].bits = tbl[p].cpy[o].bits = LARGE_COST;
  }
  /* Initialize last node to real EOF costs */
  for (o = 0; o < MAX_OFFSET; o++) {
    tbl[size].lit[o].bits = tbl[size].cpy[o].bits = EOF_COST;
  }

  /* For each position consider cost with both literal and copy blocks */
  for (p = size - 1; p >= 0; p--) {
    /* Walk backwards to each previous instance of the byte */
    for (i = tbl[p].last; i >= 0 && p - i <= MAX_OFFSET; i = tbl[i].last) {
      int off = p - i - 1; /* MAX_OFFSET is EOF in decoder, so subtract 1 */
      int len = 0;
      while (p + len < size && tbl[p + len].byte == tbl[i + len].byte) {
        len++;
        for (o = 0; o < MAX_OFFSET; o++) {
          update(&tbl[p].cpy[o], len, off, 0,
           CPY_COST(off != o, len) + tbl[p + len].lit[off].bits);
          /* OK to skip literal block if next copy updates offset */
          if (off != tbl[p + len].cpy[off].off) {
            update(&tbl[p].cpy[o], len, off, 1,
             CPY_COST(off != o, len) + tbl[p + len].cpy[off].bits);
          }
        }
      }
    }
    /* Cost to code literal */
    for (i = 1; i <= size - p; i++) {
      for (o = 0; o < MAX_OFFSET; o++) {
        update(&tbl[p].lit[o], i, o, 1, LIT_COST(i) + tbl[p + i].cpy[o].bits);
      }
    }
  }
  /* No leading bit on first literal */
  for (o = 0; o < MAX_OFFSET; o++) tbl[0].lit[o].bits--;
  return tbl[0].lit[0].bits;
}

static int pack(unsigned char *const out, const entry_t *const tbl,
 const int size) {
  int b, p, o, m, s, i, bits;

#define write_byte(b) \
  do { \
    out[s++] = b; \
    bits -= 8; \
  } \
  while (0)

#define write_bit(b) \
  do { \
    if (!m) { \
      m = 128; \
      i = s; \
      s++; \
    } \
    if (b) out[i] |= m; \
    m >>= 1; \
    bits--; \
  } \
  while (0)

#define write_length(l) \
  do { \
    int j; \
    j = 1 << msb(l); \
    while (j >>= 1) { \
      write_bit(1); \
      write_bit(l & j); \
    } \
    write_bit(0); \
  } \
  while (0)

#define ZPACK_CHECK_BITS(b) \
 ZPACK_ERROR(bits != b, ("Expected %i bits, found %i", b, bits))

  b = p = o = m = s = i = 0;
  bits = tbl[p].lit[o].bits;
  while (p < size) {
    const cost_t *c = b ? &tbl[p].cpy[o] : &tbl[p].lit[o];
    ZPACK_CHECK_BITS(c->bits);
    /* Emit literal block */
    if (b == 0) {
      /* First block is always a literal with implicit leading 0 */
      if (p > 0) write_bit(0);
      write_length(c->len);
      for (b = 0; b < c->len; b++) write_byte(tbl[p + b].byte);
    }
    /* Emit copy block */
    else {
      if (o == c->off) write_bit(0);
      else {
        write_bit(1);
        write_byte(MAX_OFFSET - (c->off + 1));
      }
      write_length(c->len);
    }
    p += c->len;
    o = c->off;
    b = c->next;
  }
  ZPACK_CHECK_BITS(EOF_COST);
  /* Write EOF flag */
  write_bit(1);
  write_byte(MAX_OFFSET);
  return s;
}

int compress(unsigned char *const out, const unsigned char *const in,
 const int size) {
  static entry_t tbl[DATA_MAX + 1];
  int p, i;
  for (p = 0; p < size; p++) {
    tbl[p].byte = in[p];
    /* Find the last position this byte was seen */
    for (i = p - 1; i >= 0 && tbl[i].byte != tbl[p].byte; i--);
    tbl[p].last = i;
  }
  int bits = compute(tbl, size);
  fprintf(stderr, "Encoded size: %i bits\n", bits);
  fprintf(stderr, "Packed ratio: %i/%i (%0.2f%%)\n",
   (bits + 7) >> 3, size, 100.0f*((bits + 7) >> 3)/size);
  return pack(out, tbl, size);
}
