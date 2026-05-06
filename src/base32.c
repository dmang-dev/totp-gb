#include "base32.h"

static int8_t b32_val(char c) {
    if (c >= 'A' && c <= 'Z') return (int8_t)(c - 'A');
    if (c >= 'a' && c <= 'z') return (int8_t)(c - 'a');
    if (c >= '2' && c <= '7') return (int8_t)(c - '2' + 26);
    return -1;
}

uint8_t base32_decode(const char *in, uint8_t in_len, uint8_t *out) {
    uint8_t out_len = 0;
    uint16_t bits = 0;
    uint8_t  n_bits = 0;
    uint8_t  i;

    for (i = 0; i < in_len; i++) {
        int8_t v;
        if (in[i] == '=') break;   /* padding */
        v = b32_val(in[i]);
        if (v < 0) return 0;       /* invalid character */
        bits   = (bits << 5u) | (uint8_t)v;
        n_bits += 5u;
        if (n_bits >= 8u) {
            n_bits -= 8u;
            out[out_len++] = (uint8_t)(bits >> n_bits);
            bits &= (uint16_t)((1u << n_bits) - 1u);
        }
    }
    return out_len;
}
