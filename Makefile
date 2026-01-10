BIN := bin
SRC := src

AS = nasm
DB = dosbox
ZP = $(BIN)/zpack

PROG := $(BIN)/logo.com

BINS := $(ZP)
OBJS := $(patsubst $(SRC)/%.c,$(BIN)/%.o,$(filter-out $(patsubst \
 $(BIN)/%,$(SRC)/%.c,$(BINS)),$(wildcard $(SRC)/*.c)))
ASMS := $(shell find $(SRC) -type f -name "*.asm")
COMS := $(patsubst $(SRC)/%.asm,$(BIN)/%.com,$(ASMS))

ORIGIN ?= 0x500

CFLAGS := -O2 -std=c89 -Wno-overlength-strings -Wall -Werror
ASFLAGS := -werror
ZPFLAGS := -O $(ORIGIN)

LIBS := -lm

ifeq (run, $(firstword $(MAKECMDGOALS)))
  ifneq (1, $(words $(MAKECMDGOALS)))
    PROG := $(BIN)/$(word 2, $(MAKECMDGOALS)).com
  endif
endif

all: $(BINS) $(OBJS)

guard=@mkdir -p $(@D)

$(BIN)/%.o: $(SRC)/%.c
	$(guard)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN)/%.com: $(SRC)/%.asm
	$(guard)
	$(AS) $(ASFLAGS) -o $@ $<

$(BIN)/z%.com: $(SRC)/%.asm $(ZP)
	$(AS) $(ASFLAGS) -DORIGIN=$(ORIGIN) -o $@ $<
	$(ZP) -o $@ $(ZPFLAGS) $@

$(SRC)/stubs.h: $(BIN)/stub.com
	@echo '/* Generated file, do not commit */' > $@
	@$(foreach s,$(patsubst $(BIN)/%.com,%,$^), \
		$(eval SIZE = $(shell stat -c %s $(BIN)/$s.com)) \
		$(eval NAME = $(shell echo $s | tr '[.a-z]' '[_A-Z]')) \
		echo 'const zpack_stub $(NAME) = {' >> $@; \
		echo '  "$s.o",' >> $@; \
		echo '  $(SIZE),' >> $@; \
		echo '  (const unsigned char[]) {' >> $@; \
		xxd -i - < $(BIN)/$s.com | sed -e s'/^/  /' >> $@; \
		echo '  }' >> $@; \
		echo '};' >> $@; \
	)

$(BIN)/%: $(SRC)/%.c $(SRC)/stubs.h $(OBJS)
	$(guard)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LIBS)

run: $(PROG)
	$(DB) -c cls $(PROG)

clean:
	rm -rf $(BIN) $(SRC)/stubs.h
