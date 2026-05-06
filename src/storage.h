#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

#define MAX_ACCOUNTS    8
#define ACCOUNT_NAME_LEN 16   /* including null terminator */
#define SECRET_B32_LEN   32   /* max base32 secret chars (covers 20 raw bytes) */

/*
 * SRAM layout (battery-backed 8KB at 0xA000):
 *
 *   0x0000  magic[2]         0xAA 0x55
 *   0x0002  count            number of stored accounts (0-8)
 *   0x0003  base_epoch[4]    uint32_t, Unix timestamp at last time-sync
 *   0x0007  reserved[1]
 *   0x0008  accounts[8]      each ACCOUNT_RECORD_SIZE bytes
 *
 * Account record:
 *   name[16]   null-terminated label
 *   secret[32] null-terminated base32 secret string
 *   Total: 48 bytes per account  →  8 * 48 = 384 bytes
 *
 * Total SRAM used: 8 + 384 = 392 bytes (well within 8KB)
 */

typedef struct {
    char    name[ACCOUNT_NAME_LEN];
    char    secret[SECRET_B32_LEN];
} Account;

/* Returns 1 if SRAM has valid data, 0 if it needs initialization */
uint8_t storage_init(void);

uint8_t storage_count(void);

void storage_get(uint8_t idx, Account *out);
void storage_set(uint8_t idx, const Account *in);

/* Append a new account; returns 1 on success, 0 if full */
uint8_t storage_add(const Account *acct);

/* Remove account at idx, shifting others down */
void storage_remove(uint8_t idx);

uint32_t storage_get_epoch(void);
void     storage_set_epoch(uint32_t epoch);

/* Palette index (0..N-1) — only meaningful on GBC builds, but harmless on DMG */
uint8_t storage_get_palette(void);
void    storage_set_palette(uint8_t idx);

#endif
