#include <stdarg.h>
#include <stdio.h>
#include "internal.h"

void zpack_log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}
