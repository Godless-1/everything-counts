# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
# =========================================================================
#  EVERYTHING COUNTS  --  Game Boy Advance
# =========================================================================
# Use arm-none-eabi-gcc from PATH when it is there; otherwise fall back to a
# toolchain unpacked under ~/.local (see README).  Override with:
#     make TOOLCHAIN=/path/to/toolchain/usr
LOCAL_TC  := $(HOME)/.local/share/gba-toolchain/root/usr
TOOLCHAIN ?= $(if $(shell command -v arm-none-eabi-gcc 2>/dev/null),,$(LOCAL_TC))
PREFIX    := $(if $(TOOLCHAIN),$(TOOLCHAIN)/bin/,)arm-none-eabi-
CC        := $(PREFIX)gcc
AS        := $(PREFIX)gcc
LD        := $(PREFIX)gcc
OBJCOPY   := $(PREFIX)objcopy
SIZE      := $(PREFIX)size

TARGET    := everything-counts
BUILD     := build
SRC       := src
GEN       := src/gen
DIST      := dist

ARCH      := -mthumb -mthumb-interwork -mcpu=arm7tdmi -mtune=arm7tdmi
WARN      := -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
             -Wno-missing-field-initializers
CFLAGS    := $(ARCH) $(WARN) -O2 -fomit-frame-pointer \
             -ffreestanding -fno-strict-aliasing -std=gnu11 \
             -fno-builtin-memcpy -fno-builtin-memset \
             -I$(SRC) -I$(GEN)
ASFLAGS   := $(ARCH) -x assembler-with-cpp
LDFLAGS   := $(ARCH) -nostdlib -Wl,-T,$(SRC)/gba.ld -Wl,--gc-sections \
             -Wl,-Map,$(BUILD)/$(TARGET).map

GENSRC    := $(GEN)/gen_font.c $(GEN)/gen_art.c
GENHDR    := $(GEN)/gen_font.h $(GEN)/gen_art.h
CSRC      := $(wildcard $(SRC)/*.c) $(GENSRC)
SSRC      := $(SRC)/crt0.s
OBJ       := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(CSRC))) \
             $(patsubst %.s,$(BUILD)/%.o,$(notdir $(SSRC)))

VPATH     := $(SRC) $(GEN)

.PHONY: all clean run debug test gen
.SUFFIXES:

all: $(DIST)/$(TARGET).gba

# ---- directories (order-only) -------------------------------------------
$(BUILD) $(DIST) $(GEN):
	@mkdir -p $@

# ---- generated assets ---------------------------------------------------
gen: $(GENSRC) $(GENHDR)

$(GEN)/gen_font.c: tools/mkfont.py | $(GEN)
	@echo "  GEN   font"
	@python3 tools/mkfont.py $(GEN)
$(GEN)/gen_font.h: $(GEN)/gen_font.c
	@:

$(GEN)/gen_art.c: tools/mkart.py | $(GEN)
	@echo "  GEN   art"
	@python3 tools/mkart.py $(GEN)
$(GEN)/gen_art.h: $(GEN)/gen_art.c
	@:

# ---- compile ------------------------------------------------------------
$(BUILD)/%.o: %.c $(GENHDR) | $(BUILD)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s | $(BUILD)
	@echo "  AS    $<"
	@$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJ) $(SRC)/gba.ld | $(BUILD)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) $(OBJ) -lgcc -o $@
	@$(SIZE) $@

$(DIST)/$(TARGET).gba: $(BUILD)/$(TARGET).elf | $(DIST)
	@echo "  OBJC  $@"
	@$(OBJCOPY) -O binary $< $@
	@python3 tools/gbafix.py $@

# ---- extras -------------------------------------------------------------
debug: gen
	@$(MAKE) -s BUILD=build-dbg TARGET=$(TARGET)-debug \
	       CFLAGS='$(CFLAGS) -DEC_DEBUG -DEC_DEBUG_NIGHT=6' all

test:
	@bash test/host/run.sh

run: all
	@bash test/run.sh

clean:
	@rm -rf $(BUILD) build-dbg $(DIST) $(GEN)
