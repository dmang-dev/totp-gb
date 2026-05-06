#ifndef BASE32_H
#define BASE32_H

#include <stdint.h>

/*
 * Decode a base32 string (RFC 4648, uppercase A-Z + 2-7, no padding required).
 * Returns number of decoded bytes written to out, or 0 on error.
 * out must be at least ceil(in_len * 5 / 8) bytes.
 */
uint8_t base32_decode(const char *in, uint8_t in_len, uint8_t *out);

#endif
