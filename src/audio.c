/*
 * Tiny SFX layer using the GB APU.
 *
 * Each sfx_*() routine writes a few NRxx registers to fire a short tone on
 * channel 1 (square w/ sweep) or 4 (noise). The notes use length-counter
 * stop, so we don't need to manually halt them.
 *
 * Frequency is encoded as 11-bit value F where:
 *     hz = 131072 / (2048 - F)
 * So F = 2048 - 131072/hz.
 */
#include "audio.h"
#include <gb/gb.h>

static uint8_t g_enabled = 1u;

void audio_init(void) {
    NR52_REG = 0x80u;            /* APU on */
    NR50_REG = 0x77u;            /* both terminals max volume */
    NR51_REG = 0xFFu;            /* route every channel to both terminals */
}

void audio_set_enabled(uint8_t on) { g_enabled = on ? 1u : 0u; }
uint8_t audio_is_enabled(void)     { return g_enabled; }

/* Channel 1 square tone for `frames` × ~4 ms with envelope. */
static void play_tone(uint16_t freq, uint8_t length, uint8_t env) {
    if (!g_enabled) return;
    NR10_REG = 0x00u;                       /* no sweep */
    NR11_REG = (uint8_t)(0x80u | length);   /* 50% duty + length load */
    NR12_REG = env;                         /* envelope: VVVV-DPPP   */
    NR13_REG = (uint8_t)(freq & 0xFFu);     /* freq lo */
    NR14_REG = (uint8_t)(0xC0u | (uint8_t)((freq >> 8) & 0x07u));
                                            /* trigger + length-enable */
}

/* Channel 4 noise burst */
static void play_noise(uint8_t length, uint8_t env, uint8_t poly) {
    if (!g_enabled) return;
    NR41_REG = (uint8_t)(0x3Fu & length);
    NR42_REG = env;
    NR43_REG = poly;
    NR44_REG = 0xC0u;                        /* trigger + length-enable */
}

void sfx_click(void)   { play_tone(1900u, 60u, 0xA1u); }   /* short high blip */
void sfx_confirm(void) { play_tone(1400u, 40u, 0xC2u); }   /* lower thump     */
void sfx_error(void)   { play_noise(50u, 0xC2u, 0x55u); }  /* harsh buzz      */
void sfx_flip(void)    { play_tone(1700u, 50u, 0x83u); }   /* TOTP roll-over  */
