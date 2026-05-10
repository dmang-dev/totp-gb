# totp-gb -- GBDK-2020 makefile
#
# Default target builds DMG (.gb), CGB (.gbc), and the seeded test
# build (.gb with TEST_SEED_ON_BOOT). Used by both local Linux/macOS
# work and GitHub Actions CI; the Windows side has build.bat.
#
# Usage:
#   make                      build all three
#   make dmg | gbc | test     build one
#   make GBDK_DIR=/opt/gbdk   override toolchain location
#   make clean
#

GBDK_DIR ?= ./gbdk
CC       := $(GBDK_DIR)/bin/lcc
ARTIFACTS := artifacts

SRCS := \
    src/main.c    src/sha1.c   src/hmac.c   src/base32.c \
    src/rtc.c     src/storage.c src/totp.c   src/input.c \
    src/ui.c      src/audio.c  src/sgb_border.c

# Cartridge header flags:
#   -Wl-yt0x10  MBC3+TIMER+RAM+BATTERY (type 0x10)
#   -Wl-ya1     1 RAM bank (8 KB SRAM)
#   -Wl-yo2     2 ROM banks (32 KB)
LFLAGS_BASE := -Wl-yt0x10 -Wl-ya1 -Wl-yo2
MFLAGS      := -Wm-ys

CFLAGS      := -Wf-MMD

DMG_OUT  := totp-gb.gb
GBC_OUT  := totp-gbc.gbc
TEST_OUT := totp-gb-test.gb

.PHONY: all dmg gbc test clean check-gbdk artifacts

all: dmg gbc test

check-gbdk:
	@test -x "$(CC)" || { \
	    echo "ERROR: GBDK-2020 not found at $(GBDK_DIR)/bin/lcc"; \
	    echo "Download from https://github.com/gbdk-2020/gbdk-2020/releases"; \
	    echo "or set GBDK_DIR=<path>."; exit 1; }

artifacts:
	@mkdir -p $(ARTIFACTS)

dmg: check-gbdk artifacts
	@echo "[DMG] Building $(DMG_OUT)..."
	"$(CC)" $(CFLAGS) $(LFLAGS_BASE) $(MFLAGS) -o $(DMG_OUT) $(SRCS)
	cp -f $(DMG_OUT) $(ARTIFACTS)/$(DMG_OUT)

gbc: check-gbdk artifacts
	@echo "[GBC] Building $(GBC_OUT)..."
	"$(CC)" $(CFLAGS) $(LFLAGS_BASE) $(MFLAGS) -Wm-yc -Wf-DGBC_BUILD \
	    -o $(GBC_OUT) $(SRCS)
	cp -f $(GBC_OUT) $(ARTIFACTS)/$(GBC_OUT)

test: check-gbdk artifacts
	@echo "[TEST] Building $(TEST_OUT) (TEST_SEED_ON_BOOT)..."
	"$(CC)" $(CFLAGS) $(LFLAGS_BASE) $(MFLAGS) -Wf-DTEST_SEED_ON_BOOT \
	    -o $(TEST_OUT) $(SRCS)
	cp -f $(TEST_OUT) $(ARTIFACTS)/$(TEST_OUT)

clean:
	rm -f $(DMG_OUT) $(GBC_OUT) $(TEST_OUT)
	rm -f *.ihx *.map *.sym *.noi *.cdb *.lst *.rst *.asm
	find src -name '*.o' -delete 2>/dev/null || true
	find src -name '*.d' -delete 2>/dev/null || true
