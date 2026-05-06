#include "sha1.h"
#include <string.h>

/* SDCC/Z80: 32-bit rotate left */
#define ROL32(x,n) (((uint32_t)(x) << (n)) | ((uint32_t)(x) >> (32u - (n))))

static const uint32_t K[4] = {
    0x5A827999UL, 0x6ED9EBA1UL, 0x8F1BBCDCUL, 0xCA62C1D6UL
};

/*
 * W[] uses a 16-entry sliding window (saves 256 bytes of stack vs W[80]).
 * Made static so SHA1 is not reentrant, which is fine on single-threaded GB.
 */
static uint32_t W[16];

static void sha1_compress(SHA1_CTX *ctx, const uint8_t *blk) {
    uint32_t a, b, c, d, e, temp, f, k;
    uint8_t i;

    for (i = 0; i < 16u; i++) {
        W[i] = ((uint32_t)blk[i*4u]     << 24u) |
               ((uint32_t)blk[i*4u + 1u] << 16u) |
               ((uint32_t)blk[i*4u + 2u] <<  8u) |
                (uint32_t)blk[i*4u + 3u];
    }

    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2];
    d = ctx->state[3]; e = ctx->state[4];

    for (i = 0; i < 80u; i++) {
        if (i >= 16u) {
            uint8_t j = (uint8_t)(i & 0x0Fu);
            W[j] = ROL32(W[(i-3u)  & 0x0Fu] ^
                         W[(i-8u)  & 0x0Fu] ^
                         W[(i-14u) & 0x0Fu] ^
                         W[j], 1u);
        }

        /*
         * SHA1 rotates K every 20 rounds — NOT every 32.
         * Selecting it via `i / 20` rather than `i >> 5` is critical;
         * an earlier `i >> 5` bug used the wrong constant for rounds
         * 20-31 and 60-63, corrupting every hash.
         */
        if      (i < 20u) { f = (b & c) | ((~b) & d);          k = K[0]; }
        else if (i < 40u) { f = b ^ c ^ d;                     k = K[1]; }
        else if (i < 60u) { f = (b & c) | (b & d) | (c & d);   k = K[2]; }
        else              { f = b ^ c ^ d;                     k = K[3]; }

        temp = ROL32(a, 5u) + f + e + k + W[i & 0x0Fu];
        e = d; d = c; c = ROL32(b, 30u); b = a; a = temp;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c;
    ctx->state[3] += d; ctx->state[4] += e;
}

void sha1_init(SHA1_CTX *ctx) {
    ctx->state[0] = 0x67452301UL;
    ctx->state[1] = 0xEFCDAB89UL;
    ctx->state[2] = 0x98BADCFEUL;
    ctx->state[3] = 0x10325476UL;
    ctx->state[4] = 0xC3D2E1F0UL;
    ctx->count    = 0;
    ctx->buf_len  = 0;
}

void sha1_update(SHA1_CTX *ctx, const uint8_t *data, uint16_t len) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        ctx->buffer[ctx->buf_len++] = data[i];
        ctx->count++;
        if (ctx->buf_len == 64u) {
            sha1_compress(ctx, ctx->buffer);
            ctx->buf_len = 0;
        }
    }
}

void sha1_final(SHA1_CTX *ctx, uint8_t digest[20]) {
    uint32_t bit_count;
    uint8_t i;

    ctx->buffer[ctx->buf_len++] = 0x80u;

    if (ctx->buf_len > 56u) {
        while (ctx->buf_len < 64u) ctx->buffer[ctx->buf_len++] = 0;
        sha1_compress(ctx, ctx->buffer);
        ctx->buf_len = 0;
    }
    while (ctx->buf_len < 56u) ctx->buffer[ctx->buf_len++] = 0;

    /* Append bit length as big-endian 64-bit; high 32 bits are always 0 here */
    bit_count = ctx->count << 3u;
    ctx->buffer[56] = 0; ctx->buffer[57] = 0;
    ctx->buffer[58] = 0; ctx->buffer[59] = 0;
    ctx->buffer[60] = (uint8_t)(bit_count >> 24u);
    ctx->buffer[61] = (uint8_t)(bit_count >> 16u);
    ctx->buffer[62] = (uint8_t)(bit_count >>  8u);
    ctx->buffer[63] = (uint8_t)(bit_count);
    sha1_compress(ctx, ctx->buffer);

    for (i = 0; i < 5u; i++) {
        digest[i*4u]     = (uint8_t)(ctx->state[i] >> 24u);
        digest[i*4u + 1u] = (uint8_t)(ctx->state[i] >> 16u);
        digest[i*4u + 2u] = (uint8_t)(ctx->state[i] >>  8u);
        digest[i*4u + 3u] = (uint8_t)(ctx->state[i]);
    }
}
