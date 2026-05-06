#ifndef TOTP_H
#define TOTP_H

#include <stdint.h>

/*
 * Compute a 6-digit TOTP code (RFC 6238).
 * secret_b32 : null-terminated base32 secret string
 * unix_time  : current Unix timestamp
 * Returns the 6-digit OTP value (0-999999), or 0xFFFFFFFF on error.
 */
uint32_t totp_generate(const char *secret_b32, uint32_t unix_time);

/*
 * Seconds remaining in the current 30-second TOTP window.
 */
uint8_t totp_seconds_remaining(uint32_t unix_time);

#endif
