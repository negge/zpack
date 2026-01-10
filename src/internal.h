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
  const unsigned char *buf;
};

int compress(unsigned char *const out, const unsigned char *const in,
 const int size);

int decompress(unsigned char *const out, const unsigned char *const in,
 const int size);

#endif
