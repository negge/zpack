#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "io.h"
#include "stubs.h"

#define PROG_ORG (1)

const char *OPTSTRING = "o:O:cpDh";

const struct option OPTIONS[] = {
  { "output",  required_argument, NULL, 'o' },
  { "origin",  required_argument, NULL, 'O' },
  { "check",   no_argument,       NULL, 'c' },
  { "payload", no_argument,       NULL, 'p' },
  { "decode",  no_argument,       NULL, 'D' },
  { "help",    no_argument,       NULL, 'h' },
  { NULL,      0,                 NULL,  0  }
};

static void usage(const char *argv0) {
  fprintf(stderr, "Usage: %s [options] <binary>\n\n"
   "Options: \n\n"
   "  -o --output <program>           Output file name for packed program\n"
   "  -O --origin <address>           Program base address (default 0x1100)\n"
   "  -c --check                      Compress stdin and print statistics\n"
   "  -p --payload                    Write out just the payload\n"
   "  -D --decode                     Assume packed input and decompress\n"
   "  -h --help                       Display this help and exit\n",
   argv0);
}

/* Identify stub used in packed binary */
static const zpack_stub *find_stub(const unsigned char *const in, short org) {
  int i;
  for (i = 0; i < sizeof(ZPACK_STUBS)/sizeof(zpack_stub); i++) {
    const zpack_stub *stub = &ZPACK_STUBS[i];
    /* Set the origin */
    *(short *)&stub->buf[PROG_ORG] = org;
    if (!memcmp(in, stub->buf, stub->size)) return stub;
  }
  return NULL;
}

#define MOD_PAYLOAD (0x1)
#define MOD_DECODE  (0x2)

int main(int argc, char *argv[]) {
  int c;
  int opt_index;
  char *output;
  char *input;
  static unsigned char in[DATA_MAX];
  int size;
  short origin;
  int flags;
  const zpack_stub *stub;
  static unsigned char out[DATA_MAX];
  zpack_stats stats;
  /* Parse the parameters */
  output = NULL;
  input = NULL;
  origin = 0x1100;
  flags = 0;
  while ((c = getopt_long(argc, argv, OPTSTRING, OPTIONS, &opt_index)) != EOF) {
    switch (c) {
      case 'o' : {
        output = optarg;
        break;
      }
      case 'O' : {
        origin = strtol(optarg, NULL, 0);
        break;
      }
      case 'c' : {
        input = "-";
        break;
      }
      case 'p' : {
        flags |= MOD_PAYLOAD;
        break;
      }
      case 'D' : {
        flags |= MOD_DECODE;
        break;
      }
      case 'h' :
      default : {
        usage(argv[0]);
        return EXIT_FAILURE;
      }
    }
  }
  if (argc - optind > 1) {
    fprintf(stderr, "More than one <binary> specified!\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (input == NULL && output == NULL) {
    fprintf(stderr, "Output -o <program> is required!\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (input == NULL && optind == argc) {
    fprintf(stderr, "No <binary> specified!\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (input != NULL && optind != argc) {
    fprintf(stderr, "Do not provide <binary> with -c --check!\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (optind != argc) {
    input = argv[optind];
  }
  /* Load the data */
  size = load(input, in, DATA_MAX);

  if (flags & MOD_DECODE) {
    if (flags & MOD_PAYLOAD) decompress(out, in, size, &stats);
    else {
      /* Read the origin */
      origin = *(short *)&in[PROG_ORG];
      stub = find_stub(in, origin);
      ZPACK_ERROR(!stub, ("File '%s' not compressed with zpack", input));
      /* Print origin */
      fprintf(stderr, "Program org: 0x%x\n", origin);
      /* Skip over stub */
      decompress(out, in + stub->size, size - stub->size, &stats);
    }
  }
  else {
    static unsigned char dec[DATA_MAX];
    decompress(dec, out, compress(out, in, size), &stats);

    /* Test decoded payload matches input */
    ZPACK_ERROR(size != stats.size,
     ("Decoded size %i does not match input size %i", stats.size, size));
    ZPACK_ERROR(memcmp(in, dec, size), ("Decoded output does not match input"));

    /* Fixup the program origin */
    stub = &ZPACK_STUBS[0];
    *(short *)&stub->buf[PROG_ORG] = origin;
  }

  /* Print statistics */
  fprintf(stderr, "Encoded size: %i bits\n", stats.bits);
  fprintf(stderr, "Packed ratio: %i/%i (%0.2f%%)\n", stats.packed, stats.size,
   100.0f*stats.packed/stats.size);

#define print_stat(l, h) \
 fprintf(stderr,"%9s: %3i, %4i bits, decodes %4i bytes (%0.2f%%)\n", \
  l, h.cnt, h.bits, h.bytes, 100.0f*h.bits/(h.bytes*8))

  print_stat("Literal", stats.hist[LIT]);
  print_stat("Update", stats.hist[UPD]);
  print_stat("Copy", stats.hist[CPY]);
  fprintf(stderr, "%9s: %3i, %4i bits\n", "EOF", 1, 9);
  fprintf(stderr, "%9s: %3i, %4i bits\n", "Total", stats.blocks, stats.bits);

  if (flags & MOD_PAYLOAD) {
    fprintf(stderr, "Decoded size: %i bytes\n", stats.size);
  }
  else {
    fprintf(stderr, "Decoder size: %i bytes\n", stub->size);
    int size = stats.packed + stub->size;
    fprintf(stderr, "Binary ratio: %i/%i (%0.2f%%)\n", size, stats.size,
     100.f*size/stats.size);
  }

  if (output != NULL) {
    /* Write decoded payload */
    if (flags & MOD_DECODE) store(output, out, stats.size);
    /* Write encoded binary */
    else {
      static unsigned char buf[DATA_MAX];
      int len = 0;

#define write_buf(b, l) \
  do { \
    memcpy(&buf[len], b, l); \
    len += l; \
  } \
  while (0)

      /* Write the decoder stub if not -p --payload */
      if (!(flags & MOD_PAYLOAD)) write_buf(stub->buf, stub->size);
      /* Write the payload */
      write_buf(out, stats.packed);

      store(output, buf, len);
    }
  }
  return EXIT_SUCCESS;
}
