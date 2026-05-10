#include "storage.h"
#include <gb/gb.h>
#include <string.h>

/* SRAM is accessed at 0xA000-0xBFFF after enabling it. */
#define MBC3_RAM_ENABLE  (*(volatile uint8_t*)0x0000u)
#define MBC3_BANK_SEL    (*(volatile uint8_t*)0x4000u)

#define MAGIC_0  0xAAu
#define MAGIC_1  0x55u

/* SRAM offsets */
#define OFF_MAGIC    0u
#define OFF_COUNT    2u
#define OFF_EPOCH    3u
#define OFF_PALETTE  7u   /* uses the previously-reserved byte */
#define OFF_ACCOUNTS 8u
#define ACCOUNT_RECORD_SIZE ((uint8_t)(ACCOUNT_NAME_LEN + SECRET_B32_LEN))

static void sram_enable(void) {
    MBC3_RAM_ENABLE = 0x0Au;
    MBC3_BANK_SEL   = 0x00u;   /* bank 0 for data */
}
static void sram_disable(void) { MBC3_RAM_ENABLE = 0x00u; }

static volatile uint8_t *sram_ptr(uint16_t off) {
    return (volatile uint8_t*)(0xA000u + off);
}

static uint8_t sram_read(uint16_t off) {
    return *sram_ptr(off);
}
static void sram_write(uint16_t off, uint8_t val) {
    *sram_ptr(off) = val;
}

static uint32_t sram_read32(uint16_t off) {
    return ((uint32_t)sram_read(off)     << 24u) |
           ((uint32_t)sram_read(off+1u)  << 16u) |
           ((uint32_t)sram_read(off+2u)  <<  8u) |
            (uint32_t)sram_read(off+3u);
}
static void sram_write32(uint16_t off, uint32_t v) {
    sram_write(off,     (uint8_t)(v >> 24u));
    sram_write(off+1u,  (uint8_t)(v >> 16u));
    sram_write(off+2u,  (uint8_t)(v >>  8u));
    sram_write(off+3u,  (uint8_t)(v));
}

uint8_t storage_init(void) {
    uint8_t ok;
    sram_enable();
    ok = (sram_read(OFF_MAGIC)   == MAGIC_0 &&
          sram_read(OFF_MAGIC+1u) == MAGIC_1);
    if (!ok) {
        /* First boot: format SRAM */
        sram_write(OFF_MAGIC,    MAGIC_0);
        sram_write(OFF_MAGIC+1u, MAGIC_1);
        sram_write(OFF_COUNT,    0u);
        sram_write32(OFF_EPOCH,  0UL);
    }

#ifdef TEST_SEED_ON_BOOT
    /*
     * Test build: unconditionally re-seed SRAM at every boot with a known
     * fixture (epoch + JBSWY3DPEHPK3PXP "Hello!" account at slot 0). Lets
     * the mGBA MCP harness verify code generation against a known answer
     * without having to drive the UI to add an account.
     *
     * Seeded epoch  : 1778088090  (2026-05-06 17:21:30 UTC)
     * Expected code : 283711      (T window 59269603)
     */
    {
        static const char SEED_NAME[ACCOUNT_NAME_LEN] = "Test";
        static const char SEED_SECRET[SECRET_B32_LEN] = "JBSWY3DPEHPK3PXP";
        uint8_t i;
        sram_write(OFF_MAGIC,    MAGIC_0);
        sram_write(OFF_MAGIC+1u, MAGIC_1);
        sram_write(OFF_COUNT,    1u);
        sram_write32(OFF_EPOCH,  1778088090UL);
        for (i = 0; i < ACCOUNT_NAME_LEN; i++)
            sram_write(OFF_ACCOUNTS + i, (uint8_t)SEED_NAME[i]);
        for (i = 0; i < SECRET_B32_LEN; i++)
            sram_write(OFF_ACCOUNTS + ACCOUNT_NAME_LEN + i, (uint8_t)SEED_SECRET[i]);
        ok = 1u;   /* report 'returning user' so main skips the welcome screen */
    }
#endif

    sram_disable();
    return ok;
}

uint8_t storage_count(void) {
    uint8_t n;
    sram_enable();
    n = sram_read(OFF_COUNT);
    sram_disable();
    return n;
}

void storage_get(uint8_t idx, Account *out) {
    uint16_t base = OFF_ACCOUNTS + (uint16_t)idx * ACCOUNT_RECORD_SIZE;
    uint8_t i;
    sram_enable();
    for (i = 0; i < ACCOUNT_RECORD_SIZE; i++)
        ((uint8_t*)out)[i] = sram_read(base + i);
    sram_disable();
}

void storage_set(uint8_t idx, const Account *in) {
    uint16_t base = OFF_ACCOUNTS + (uint16_t)idx * ACCOUNT_RECORD_SIZE;
    uint8_t i;
    sram_enable();
    for (i = 0; i < ACCOUNT_RECORD_SIZE; i++)
        sram_write(base + i, ((const uint8_t*)in)[i]);
    sram_disable();
}

uint8_t storage_add(const Account *acct) {
    uint8_t n;
    sram_enable();
    n = sram_read(OFF_COUNT);
    if (n >= MAX_ACCOUNTS) { sram_disable(); return 0; }
    sram_write(OFF_COUNT, n + 1u);
    sram_disable();
    storage_set(n, acct);
    return 1;
}

void storage_remove(uint8_t idx) {
    uint8_t n, i;
    Account tmp;
    sram_enable();
    n = sram_read(OFF_COUNT);
    sram_disable();
    if (idx >= n) return;
    for (i = idx; i < n - 1u; i++) {
        storage_get(i + 1u, &tmp);
        storage_set(i, &tmp);
    }
    sram_enable();
    sram_write(OFF_COUNT, n - 1u);
    sram_disable();
}

uint32_t storage_get_epoch(void) {
    uint32_t v;
    sram_enable();
    v = sram_read32(OFF_EPOCH);
    sram_disable();
    return v;
}

void storage_set_epoch(uint32_t epoch) {
    sram_enable();
    sram_write32(OFF_EPOCH, epoch);
    sram_disable();
}

/* Settings byte layout: bits 0-6 = palette index, bit 7 = sound MUTE flag.
 * Existing saves had bit 7 clear → sound defaults to ON. */
uint8_t storage_get_palette(void) {
    uint8_t v;
    sram_enable();
    v = sram_read(OFF_PALETTE);
    sram_disable();
    return v & 0x7Fu;
}

void storage_set_palette(uint8_t idx) {
    uint8_t cur;
    sram_enable();
    cur = sram_read(OFF_PALETTE) & 0x80u;        /* preserve mute bit */
    sram_write(OFF_PALETTE, cur | (idx & 0x7Fu));
    sram_disable();
}

uint8_t storage_get_sound_enabled(void) {
    uint8_t v;
    sram_enable();
    v = sram_read(OFF_PALETTE);
    sram_disable();
    return (v & 0x80u) ? 0u : 1u;
}

void storage_set_sound_enabled(uint8_t on) {
    uint8_t cur;
    sram_enable();
    cur = sram_read(OFF_PALETTE) & 0x7Fu;        /* keep palette index */
    sram_write(OFF_PALETTE, cur | (on ? 0u : 0x80u));
    sram_disable();
}
