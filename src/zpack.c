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
  int wrote;
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
  /* Write the stub */
  stub = &STUB;
  memcpy(out, stub->buf, stub->size);
  /* Fixup the program origin */
  *(short *)&out[PROG_ORG] = origin;
  /* Load the data */
  size = load(input, in, DATA_MAX);

  if (flags & MOD_DECODE) {
    if (flags & MOD_PAYLOAD) {
      wrote = decompress(out, in, size);
    }
    else {
      /* Read the origin */
      origin = *(short *)&in[PROG_ORG];
      fprintf(stderr, "Program org: 0x%x\n", origin);
      wrote = decompress(out, in + stub->size, size - stub->size);
    }
    fprintf(stderr, "Decoded size: %i (bytes)\n", wrote);
    fprintf(stderr, "Binary ratio: %i/%i (%0.2f%%)\n", size, wrote,
     100.f*size/wrote);
  }
  else {
    wrote = compress(out + stub->size, in, size);
    int packed = wrote + stub->size;
    fprintf(stderr, "Decoder size: %i bytes\n", stub->size);
    fprintf(stderr, "Binary ratio: %i/%i (%0.2f%%)\n", packed, size,
     100.f*packed/size);
  }

  if (output != NULL) {
    if (flags & MOD_PAYLOAD || flags & MOD_DECODE) {
      store(output, out + stub->size, wrote);
    }
    else {
      store(output, out, stub->size + wrote);
    }
  }
  return EXIT_SUCCESS;
}
