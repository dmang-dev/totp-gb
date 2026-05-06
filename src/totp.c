#include "totp.h"
#include "hmac.h"
#include "base32.h"
#include <string.h>

#define TOTP_STEP 30u
#define MAX_KEY   20u   /* 160-bit max decoded secret */

static uint8_t g_key[MAX_KEY];
static uint8_t g_msg[8];     /* 64-bit counter, big-endian */
static uint8_t g_mac[20];

uint32_t totp_generate(const char *secret_b32, uint32_t unix_time) {
    uint8_t  key_len;
    uint32_t T;
    uint8_t  offset;
    uint32_t code;

    /* Decode the base32 secret */
    key_len = base32_decode(secret_b32, (uint8_t)strlen(secret_b32), g_key);
    if (key_len == 0u) return 0xFFFFFFFFUL;

    /* T = floor(unix_time / 30), packed as big-endian 8-byte value */
    T = unix_time / TOTP_STEP;
    g_msg[0] = 0; g_msg[1] = 0; g_msg[2] = 0; g_msg[3] = 0;
    g_msg[4] = (uint8_t)(T >> 24u);
    g_msg[5] = (uint8_t)(T >> 16u);
    g_msg[6] = (uint8_t)(T >>  8u);
    g_msg[7] = (uint8_t)(T);

    hmac_sha1(g_key, key_len, g_msg, 8u, g_mac);

    /* Dynamic truncation (RFC 4226 §5.3) */
    offset = g_mac[19] & 0x0Fu;
    code = ((uint32_t)(g_mac[offset]     & 0x7Fu) << 24u)
         | ((uint32_t) g_mac[offset + 1u]         << 16u)
         | ((uint32_t) g_mac[offset + 2u]         <<  8u)
         | ((uint32_t) g_mac[offset + 3u]);

    return code % 1000000UL;
}

uint8_t totp_seconds_remaining(uint32_t unix_time) {
    return (uint8_t)(TOTP_STEP - (uint8_t)(unix_time % TOTP_STEP));
}
