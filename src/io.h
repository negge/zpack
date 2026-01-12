#ifndef _IO_H
#define _IO_H

typedef struct bit_reader bit_reader;

struct bit_reader {
  /* input buffer */
  const unsigned char *in;
  int size;

  /* reader state */
  int mask;
  int idx;
  int val;

  /* consumed bits */
  int bits;
};

int load(char *const input, unsigned char *const in, const int len);
void store(char *const output, const unsigned char *const out, const int len);

void br_init(bit_reader *const br, const unsigned char *const in, int size);
unsigned char read_byte(bit_reader *const br);
int read_bit(bit_reader *const br);
int read_length(bit_reader *const br, int *bits);

#endif
