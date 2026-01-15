#ifndef _INTERNAL_H
#define _INTERNAL_H

#include <stdlib.h>

#define ZPACK_LOG(err) zpack_log err

#define ZPACK_ERROR(cond, err) \
  do { \
    if (cond) { \
      ZPACK_LOG(err); \
      exit(EXIT_FAILURE); \
    } \
  } \
  while (0)

void zpack_log(const char *fmt, ...);

#define DATA_MAX (20*1024)

typedef struct zpack_stub zpack_stub;

struct zpack_stub {
  const char *name;
  unsigned int size;
  unsigned char *buf;
};

typedef enum {
  LIT = 0,
  CPY = 1,
  UPD = 2
} block_t;

typedef struct zpack_stats zpack_stats;

struct zpack_stats {
  int size;
  int bits;
  int packed;

  struct {
    int cnt;
    int bits;
    int bytes;
  } hist[3];
  int blocks;
};

#define MOD_PAYLOAD (0x1)
#define MOD_DECODE  (0x2)
#define MOD_EXT_CPY (0x4)
#define MOD_QUIET   (0x10)

int compress(unsigned char *const out, const unsigned char *const in,
 const int size, const int flags);

void decompress(unsigned char *const out, const unsigned char *const in,
 const int size, zpack_stats *stats, const int flags);

#endif
