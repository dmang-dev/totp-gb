#ifndef HMAC_H
#define HMAC_H

#include <stdint.h>
#include "sha1.h"

/*
 * Compute HMAC-SHA1.
 * key     : raw key bytes
 * key_len : key length in bytes (max 64; longer keys are SHA1-hashed first)
 * msg     : message bytes
 * msg_len : message length in bytes
 * out     : 20-byte output buffer
 */
void hmac_sha1(const uint8_t *key, uint8_t key_len,
               const uint8_t *msg, uint8_t msg_len,
               uint8_t out[20]);

#endif
