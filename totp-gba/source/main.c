/*
 * totp-gba — minimal toolchain smoke test.
 *
 * Boots into mode 0, sets a phosphor-green palette, prints "TOTP GBA"
 * centered using libtonc's text engine. Confirms devkitARM + libtonc +
 * libgba all link and that we have a working hello-world before we port
 * any real authenticator code.
 *
 * Build:  make           (in this directory, with DEVKITARM in env)
 * Output: totp-gba.gba
 */
#include <tonc.h>

/* 5-bit BGR (BGR15) phosphor-green ramp, darkest -> brightest */
#define PHOS_DARK    RGB15( 2,  4,  2)
#define PHOS_MID_LO  RGB15( 8, 14,  8)
#define PHOS_MID_HI  RGB15(15, 25, 15)
#define PHOS_BRIGHT  RGB15(24, 31, 24)

int main(void) {
    /* Initialise libtonc's tile-text engine on BG 0 */
    tte_init_se_default(0, BG_CBB(0) | BG_SBB(31));

    /* Background colour fills the screen frame. Text colour is index 1. */
    pal_bg_mem[0] = PHOS_DARK;
    pal_bg_mem[1] = PHOS_BRIGHT;
    pal_bg_mem[2] = PHOS_MID_HI;
    pal_bg_mem[3] = PHOS_MID_LO;

    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;

    /* GBA screen is 240x160 px = 30x20 chars at 8x8 font.
     * Centre "TOTP GBA" (8 chars) at column 11, vertical centre row 9. */
    tte_set_pos(11 * 8, 9 * 8);
    tte_write("TOTP GBA");

    tte_set_pos(6 * 8, 11 * 8);
    tte_write("toolchain smoke test");

    while (1) { VBlankIntrWait(); }
    return 0;
}
