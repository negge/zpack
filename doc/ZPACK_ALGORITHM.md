# ZPACK Algorithm Documentation

## Overview

ZPACK is a post-linker binary compression tool designed for sizecoding and retro programming. It implements an optimal dynamic-programming encoder for the ZX2 compression format with a 53-byte decompression stub for MS-DOS using only 8086 instructions.

## Algorithm Description

### ZX2 Compression Format

ZPACK uses the ZX2 compression format, which is a variant of LZSS (Lempel-Ziv-Storer-Szymanski) compression. The format uses three types of blocks:

1. **Literal blocks (LIT)**: Raw uncompressed bytes
2. **Copy blocks (CPY)**: Copy bytes from previously decoded data (with same offset as previous copy)
3. **Update blocks (UPD)**: Copy bytes from previously decoded data (with new offset)
4. **EOF block**: End of compressed data marker

### Bitstream Encoding

The compressed data is encoded as a bitstream with the following structure:

- **Literal blocks**: `0` + Elias gamma encoded length + raw bytes
- **Copy blocks**: `10` + (optional offset byte) + Elias gamma encoded length
- **Update blocks**: `11` + offset byte + Elias gamma encoded length
- **EOF block**: `1` + `0xFF` byte

### Elias Gamma Encoding

Elias gamma encoding is used for encoding lengths. It's a universal code that encodes positive integers:

- `1` is encoded as `1`
- `2` is encoded as `010`
- `3` is encoded as `011`
- `4` is encoded as `00100`
- etc.

The encoding works by writing the binary representation of the number without the leading 1, followed by the binary representation with the leading 1.

### Compression Process

1. **Initialization**: Build a table of byte positions for fast lookup
2. **Dynamic Programming**: For each position, compute the optimal compression path:
   - Try literal blocks of various lengths
   - Try copy blocks from previous occurrences of the same byte
   - Choose the path with minimum bit cost
3. **Bitstream Packing**: Convert the optimal path into a compact bitstream

### Decompression Process

The decompression stub (written in 8086 assembly) works as follows:

1. Initialize destination pointer (DI) and source pointer (SI)
2. Read Elias gamma encoded length for literals
3. Copy literal bytes from source to destination
4. Check next bit:
   - `0`: Use same offset as previous copy
   - `1`: Read new offset byte and use it
5. Read Elias gamma encoded length for copy
6. Copy bytes from destination - offset to destination
7. Repeat until EOF marker is encountered

## Build System Documentation

### Current Makefile-based Build System

The current build system uses GNU Make with the following key components:

#### Variables

- `BIN`: Output directory (`bin/`)
- `SRC`: Source directory (`src/`)
- `AS`: Assembler (`nasm`)
- `DB`: DOS emulator (`dosbox`)
- `ZP`: ZPACK executable path
- `CFLAGS`: C compiler flags (`-O2 -std=c89 -Wno-overlength-strings -Wall -Werror`)
- `ASFLAGS`: Assembler flags (`-werror`)
- `LIBS`: Linker libraries (`-lm`)

#### Key Targets

1. **`all`**: Build all executables and object files
2. **`$(BIN)/%.o`**: Compile C source to object file
3. **`$(BIN)/%.com`**: Assemble NASM source to COM file
4. **`$(BIN)/stub.com`**: Build decoder stub with optional extensions
5. **`$(BIN)/z%.com`**: Build and compress example programs
6. **`$(SRC)/stubs.h`**: Generate stub header from compiled stubs
7. **`$(BIN)/%`**: Link final executables
8. **`run`**: Run example program in DOSBox
9. **`clean`**: Remove all build artifacts

#### Build Process

1. **Stubs Generation**: 
   - Assemble `stub.asm` with different configurations
   - Generate `stubs.h` header containing embedded stub data

2. **Main Executable**:
   - Compile C source files to object files
   - Link object files with `stubs.h` to create `zpack` executable

3. **Example Programs**:
   - Assemble `logo.asm` to create example COM program
   - Optionally compress with zpack to create `zlogo.com`

#### Special Features

- **Progress Bar**: The encoder shows a progress bar during compression
- **Statistics**: Detailed compression statistics are displayed
- **Multiple Stubs**: Supports different decoder stub variants
- **Origin Handling**: Programs can be compressed for specific memory origins

## File Structure

### Source Files

- `src/zpack.c`: Main program with CLI interface
- `src/encode.c`: Compression algorithm implementation
- `src/decode.c`: Decompression algorithm implementation  
- `src/io.c`: I/O utilities and bit reader
- `src/internal.h`: Internal data structures and definitions
- `src/io.h`: I/O function declarations
- `src/stub.asm`: 8086 decoder stub assembly
- `src/logo.asm`: Example DOS COM program

### Generated Files

- `src/stubs.h`: Generated header with embedded stub data
- `bin/zpack`: Main zpack executable
- `bin/logo.com`: Example uncompressed program
- `bin/zlogo.com`: Example compressed program
- `bin/stub.com`: Decoder stub
- `bin/stubx.com`: Extended decoder stub

### Documentation

- `doc/ZPACK_ALGORITHM.md`: This algorithm documentation
- `doc/logo.png`: Example program output screenshot
- `doc/zlogo.png`: Compressed example output screenshot

## Command Line Interface

### Options

- `-o, --output <program>`: Output file name
- `-O, --origin <address>`: Program base address (default 0x1100)
- `-c, --check`: Compress stdin and print statistics
- `-p, --payload`: Write out just the payload (no stub)
- `-D, --decode`: Decompress packed input
- `-x, --ext-copy`: Extend copy length by 1 (minimum of 2)
- `-q, --quiet`: Print only error messages
- `-h, --help`: Display help

### Usage Examples

```bash
# Compress a program for origin 0x500
zpack -O 0x500 -o zintro.com intro.com

# Decompress a packed program
zpack -D -o logo.com zlogo.com

# Check compression statistics
zpack -c < program.com

# Create payload only (no stub)
zpack -p -o payload.bin program.com
```

## Technical Details

### Data Structures

- `entry_t`: Dynamic programming table entry with cost calculations
- `cost_t`: Cost information for compression decisions
- `zpack_stats`: Compression statistics structure
- `bit_reader`: Bitstream reader for decompression
- `zpack_stub`: Decoder stub information

### Key Functions

- `compress()`: Main compression function
- `decompress()`: Main decompression function
- `compute()`: Dynamic programming cost computation
- `pack()`: Bitstream packing
- `unpack()`: Bitstream unpacking
- `read_bit()`: Read single bit from bitstream
- `read_length()`: Read Elias gamma encoded length
- `write_bit()`: Write single bit to bitstream
- `write_length()`: Write Elias gamma encoded length

### Memory Constraints

- `DATA_MAX`: 20KB maximum input size
- `MAX_OFFSET`: 255 byte maximum copy offset
- Stubs are optimized to be as small as possible (53 bytes for basic stub)

## Performance Characteristics

- **Compression Ratio**: Typically 25-40% for small DOS programs
- **Decoder Size**: 53 bytes for basic stub, slightly larger for extended features
- **Speed**: Optimized for small files, shows progress during compression
- **Memory Usage**: Designed for 8086 systems with limited memory

## Limitations

- Maximum input size: 20KB
- Maximum copy offset: 255 bytes
- Designed specifically for DOS COM format executables
- Requires programs to be linked for specific memory origins

## Future Enhancements

The algorithm could potentially be enhanced with:
- Larger offset support
- Different encoding strategies for the bitstream
- Adaptive compression parameters
- Support for larger input files
- Additional stub variants with different trade-offs