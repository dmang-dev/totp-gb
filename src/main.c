/*
 * TOTP GB — Time-based One-Time Password authenticator for Game Boy.
 *
 * Requires an MBC3+TIMER+RAM+BATTERY cartridge (type 0x10).
 * Tested with GBDK-2020 and mGBA / BGB emulators.
 *
 * Controls (main screen):
 *   Up/Down   : scroll account list
 *   A         : view account / refresh code
 *   B         : open time-sync screen
 *   Start     : add new account
 *   Select    : (in view screen) delete account
 */

#include <gb/gb.h>
#include <gb/sgb.h>
#include <gbdk/console.h>
#include <stdio.h>
#include <stdint.h>

/*
 * Available background palettes. Cycle on the main screen with LEFT / RIGHT;
 * the choice persists in SRAM. Two implementations:
 *
 *   GBC build  → real CGB colour palettes via set_bkg_palette()
 *   DMG build  → BGP_REG packed-grayscale variants (also pleasant on
 *                MGB / Pocket / Light, where the LCD is true grayscale and
 *                inverted / high-contrast modes look especially crisp).
 */

#ifdef GBC_BUILD
#include <gb/cgb.h>

#define NUM_PALETTES 6u
static const palette_color_t palettes[NUM_PALETTES][4] = {
    /* 0 — Phosphor green (default) */
    { RGB8( 16,  32,  16), RGB8( 64, 112,  64),
      RGB8(120, 200, 120), RGB8(192, 248, 192) },
    /* 1 — Amber CRT */
    { RGB8( 24,  12,   0), RGB8( 96,  56,   8),
      RGB8(184, 120,  24), RGB8(255, 200,  80) },
    /* 2 — Game Boy DMG */
    { RGB8( 16,  56,  16), RGB8( 48, 104,  16),
      RGB8(136, 192,  56), RGB8(176, 224, 104) },
    /* 3 — Game Boy Pocket grayscale */
    { RGB8( 24,  24,  24), RGB8( 80,  80,  80),
      RGB8(168, 168, 168), RGB8(232, 232, 232) },
    /* 4 — Cool blue */
    { RGB8( 16,  16,  56), RGB8( 56,  88, 160),
      RGB8(120, 168, 232), RGB8(216, 232, 255) },
    /* 5 — Magenta dusk */
    { RGB8( 32,   8,  40), RGB8(120,  32, 112),
      RGB8(216,  88, 184), RGB8(248, 200, 240) },
};

static void apply_palette(uint8_t idx) {
    if (idx >= NUM_PALETTES) idx = 0;
    if (_cpu == CGB_TYPE) {
        set_bkg_palette(0, 1, palettes[idx]);
    }
}

#else /* DMG / MGB build ----------------------------------------------- */

/*
 * BGP_REG packs 4 × 2-bit colour mappings: bits 0-1 = pixel value 0,
 * bits 2-3 = value 1, etc. Colour values: 0 = lightest, 3 = darkest.
 *
 *   0xE4 = 11 10 01 00  — standard (pixel 0 → light, pixel 3 → dark)
 *   0x1B = 00 01 10 11  — inverted
 *   0xFC = 11 11 11 00  — high-contrast (no antialiasing greys)
 *   0x90 = 10 01 00 00  — soft / dimmer text
 *   0x03 = 00 00 00 11  — dark mode w/ no AA (white text on solid black)
 */
#define NUM_PALETTES 5u
static const uint8_t bgp_palettes[NUM_PALETTES] = {
    0xE4u,  /* 0 — Normal           */
    0x1Bu,  /* 1 — Inverted         */
    0xFCu,  /* 2 — High Contrast    */
    0x90u,  /* 3 — Soft             */
    0x03u,  /* 4 — Dark Mode        */
};

static void apply_palette(uint8_t idx) {
    if (idx >= NUM_PALETTES) idx = 0;
    BGP_REG = bgp_palettes[idx];
}

#endif

#include "storage.h"
#include "rtc.h"
#include "totp.h"
#include "ui.h"
#include "input.h"
#include "audio.h"

/* Current Unix time = base_epoch + RTC elapsed seconds */
static uint32_t g_base_epoch;

/* Non-static so ui.c can refresh time per frame in long-lived screens. */
uint32_t current_unix_time(void) {
    return g_base_epoch + rtc_elapsed_seconds();
}

static void sync_time(void) {
    uint32_t new_epoch = ui_screen_timeset(current_unix_time());
    g_base_epoch = new_epoch;
    storage_set_epoch(new_epoch);
    rtc_reset();   /* restart RTC from 0 at this moment */
}

/*
 * Super Game Boy: send a PAL01 packet to colorize the in-game tiles
 * with the same phosphor-green palette we use on GBC. The SGB feeds
 * post-BGP_REG pixel values into its palette indexing, so DMG palette
 * cycling (Inverted / High Contrast / etc.) still works on SGB and
 * just remaps which of these four colours each pixel takes.
 *
 * SGB BGR15 colour format:  bits 0-4 = R, 5-9 = G, 10-14 = B (each 0-31).
 * Macro packs 5-bit components into a little-endian uint16 byte pair.
 */
#define SGB_RGB(r,g,b)  (uint8_t)(((g) << 5) | (r)),  \
                        (uint8_t)(((b) << 2) | ((g) >> 3))

static const uint8_t sgb_pal_packet[16] = {
    (SGB_PAL_01 << 3) | 1u,     /* command + length=1 block */
    SGB_RGB( 2,  4,  2),        /* pal 0 col 0  — darkest  (shared)   */
    SGB_RGB( 8, 14,  8),        /* pal 0 col 1                        */
    SGB_RGB(15, 25, 15),        /* pal 0 col 2                        */
    SGB_RGB(24, 31, 24),        /* pal 0 col 3  — brightest           */
    SGB_RGB( 8, 14,  8),        /* pal 1 col 1  (col 0 shared above)  */
    SGB_RGB(15, 25, 15),        /* pal 1 col 2                        */
    SGB_RGB(24, 31, 24),        /* pal 1 col 3                        */
    0x00u,                      /* padding                            */
};

static void sgb_init(void) {
    uint8_t i;
    /* SGB needs ~4 frames after boot before sgb_check works on PAL SNES */
    for (i = 0; i < 4u; i++) wait_vbl_done();
    if (sgb_check()) {
        sgb_transfer((uint8_t *)sgb_pal_packet);
    }
}

void main(void) {
    uint8_t scroll   = 0u;
    uint8_t selected = 0u;
    uint8_t count;
    uint8_t first_boot;

    /* Initialise APU for SFX */
    audio_init();

    /* Turn on display */
    DISPLAY_ON;
    SHOW_BKG;

#ifdef GBC_BUILD
    /* On real CGB hardware, switch to double-speed (2x CPU). Halves the
     * time spent in HMAC-SHA1 and makes UI feel instant. No-op on DMG. */
    if (_cpu == CGB_TYPE) cpu_fast();
#endif

    /* SGB detection + palette setup (no-op on plain DMG/MGB/CGB) */
    sgb_init();

    /* Init persistent storage (must precede any storage_get_*) */
    first_boot = !storage_init();
    g_base_epoch = storage_get_epoch();

    /* Restore the user's last-chosen palette from SRAM (GBC colour or DMG BGP) */
    apply_palette(storage_get_palette());

    /* On first boot, ask the user to set the time immediately */
    if (first_boot || g_base_epoch == 0u) {
        ui_clear();
        gotoxy(1, 5);  puts("Welcome to TOTP GB!");
        gotoxy(1, 7);  puts("Set the time first.");
        gotoxy(1, 9);  puts("Press any button.");
        input_wait_any();
        sync_time();
    }

    /* Main loop */
    {
        uint32_t prev_t      = 0xFFFFFFFFUL;
        uint32_t prev_window = 0xFFFFFFFFUL;
        uint8_t  prev_sel    = 0xFFu;
        uint8_t  prev_cnt    = 0xFFu;
        uint8_t  need_clear  = 1u;   /* clear on first entry */

        for (;;) {
            uint32_t unix_time;

            wait_vbl_done();
            input_update();
            unix_time = current_unix_time();
            count     = storage_count();

            /* Clamp selection / scroll */
            if (count == 0u) { selected = 0u; scroll = 0u; }
            if (selected >= count && count > 0u) selected = count - 1u;
            if (selected < scroll) scroll = selected;
            if (selected >= scroll + 4u) scroll = selected - 3u;

            /* Always redraw — ui_draw_main caches HMAC results internally
             * so per-frame cost is just memory writes for the ticker scroll. */
            if (need_clear) { ui_clear(); need_clear = 0u; }
            {
                uint32_t window = unix_time / 30UL;
                if (window != prev_window && prev_window != 0xFFFFFFFFUL && count > 0u) {
                    sfx_flip();
                }
                prev_window = window;
                ui_draw_main(scroll, selected, unix_time);
                prev_t = unix_time; prev_sel = selected; prev_cnt = count;
            }

        /* Navigation */
        if (input_pressed(J_UP) && selected > 0u)   { selected--; sfx_click(); }
        if (input_pressed(J_DOWN) && selected + 1u < count) { selected++; sfx_click(); }

            /* L/R cycles palettes — colour on GBC, BGP grayscale on DMG/MGB */
            if (input_pressed(J_LEFT) || input_pressed(J_RIGHT)) {
                uint8_t p = storage_get_palette();
                if (p >= NUM_PALETTES) p = 0u;
                if (input_pressed(J_LEFT))
                    p = (p == 0u) ? (NUM_PALETTES - 1u) : (uint8_t)(p - 1u);
                else
                    p = (uint8_t)((p + 1u) % NUM_PALETTES);
                storage_set_palette(p);
                apply_palette(p);
                sfx_click();
            }

            /* Add account */
            if (input_pressed(J_START)) {
                if (count < MAX_ACCOUNTS) {
                    sfx_confirm();
                    ui_screen_add();
                } else {
                    sfx_error();
                    ui_clear();
                    gotoxy(1, 8); puts("Account list full!");
                    gotoxy(1, 9); puts("Delete one first.");
                    input_wait_any();
                }
                need_clear = 1u; prev_t = 0xFFFFFFFFUL;
            }

            /* View / delete */
            if (input_pressed(J_A) && count > 0u) {
                sfx_click();
                {
                    uint8_t deleted = ui_screen_view(selected, unix_time);
                    if (deleted && selected > 0u && selected >= storage_count()) {
                        selected = storage_count();
                        if (selected > 0u) selected--;
                    }
                }
                need_clear = 1u; prev_t = 0xFFFFFFFFUL;
            }

            /* Time sync */
            if (input_pressed(J_B)) {
                sfx_confirm();
                sync_time();
                need_clear = 1u; prev_t = 0xFFFFFFFFUL;
            }
        }
    }
}
