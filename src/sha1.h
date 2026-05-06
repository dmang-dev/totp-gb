#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>

#define SHA1_DIGEST_SIZE 20
#define SHA1_BLOCK_SIZE  64

typedef struct {
    uint32_t state[5];
    uint32_t count;      /* bytes processed */
    uint8_t  buf_len;
    uint8_t  buffer[64];
} SHA1_CTX;

void sha1_init(SHA1_CTX *ctx);
void sha1_update(SHA1_CTX *ctx, const uint8_t *data, uint16_t len);
void sha1_final(SHA1_CTX *ctx, uint8_t digest[20]);

#endif
