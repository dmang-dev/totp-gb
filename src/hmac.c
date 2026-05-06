#include "hmac.h"
#include <string.h>

/* Static contexts avoid ~180 bytes of stack pressure */
static SHA1_CTX g_inner_ctx;
static SHA1_CTX g_outer_ctx;
static uint8_t  g_k[64];      /* padded key block */

void hmac_sha1(const uint8_t *key, uint8_t key_len,
               const uint8_t *msg, uint8_t msg_len,
               uint8_t out[20]) {
    uint8_t i;
    uint8_t inner_hash[20];

    /* If key > block size, hash it */
    if (key_len > 64u) {
        sha1_init(&g_inner_ctx);
        sha1_update(&g_inner_ctx, key, key_len);
        sha1_final(&g_inner_ctx, g_k);
        key_len = 20u;
    } else {
        memcpy(g_k, key, key_len);
    }
    memset(g_k + key_len, 0, 64u - key_len);

    /* Inner hash: SHA1((K XOR ipad) || msg) */
    sha1_init(&g_inner_ctx);
    for (i = 0; i < 64u; i++) g_k[i] ^= 0x36u;
    sha1_update(&g_inner_ctx, g_k, 64u);
    sha1_update(&g_inner_ctx, msg, msg_len);
    sha1_final(&g_inner_ctx, inner_hash);

    /* Restore key, apply opad */
    for (i = 0; i < 64u; i++) g_k[i] ^= (0x36u ^ 0x5Cu);

    /* Outer hash: SHA1((K XOR opad) || inner_hash) */
    sha1_init(&g_outer_ctx);
    sha1_update(&g_outer_ctx, g_k, 64u);
    sha1_update(&g_outer_ctx, inner_hash, 20u);
    sha1_final(&g_outer_ctx, out);
}
