# TOTP GB — Makefile for GBDK-2020
#
# Prerequisites:
#   GBDK-2020: https://github.com/gbdk-2020/gbdk-2020/releases
#   Set GBDK_DIR to your installation path, or pass it on the command line.
#
# Usage:
#   make GBDK_DIR=/path/to/gbdk-2020
#   make clean

# Set GBDK_DIR to your GBDK-2020 install (Windows or Linux/Mac path both work).
# Download from: https://github.com/gbdk-2020/gbdk-2020/releases
GBDK_DIR ?= I:\totp-gb\gbdk

CC   = $(GBDK_DIR)/bin/lcc
EMU  = mgba           # change to bgb, sameboy, etc.

TARGET  = totp-gb.gb

SRCS = \
    src/main.c   \
    src/sha1.c   \
    src/hmac.c   \
    src/base32.c \
    src/rtc.c    \
    src/storage.c \
    src/totp.c   \
    src/input.c  \
    src/ui.c

# Cartridge header flags:
#   -Wl-yt0x10  : MBC3 + TIMER + RAM + BATTERY  (type 0x10)
#   -Wl-ya1     : 1 RAM bank (8 KB SRAM)
#   -Wl-yo2     : 2 ROM banks (32 KB)
CFLAGS  = -Wf-MMD
LFLAGS  = -Wl-yt0x10 -Wl-ya1 -Wl-yo2

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(SRCS)

run: $(TARGET)
	$(EMU) $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(patsubst %.c,%.o,$(SRCS))
	rm -f $(patsubst %.c,%.lst,$(SRCS))
	rm -f $(patsubst %.c,%.asm,$(SRCS))
	rm -f $(patsubst %.c,%.sym,$(SRCS))
	rm -f $(patsubst %.c,%.map,$(SRCS))
	rm -f $(patsubst %.c,%.d,$(SRCS))
