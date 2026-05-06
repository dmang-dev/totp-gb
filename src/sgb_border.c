/*
 * Procedural SGB border.
 *
 * Renders a deep-blue frame around the GB screen with a phosphor-green
 * accent line just inside, matching the in-game palette. Hand-authored
 * in C (no png2asset dependency) using only 3 unique 8x8 tiles:
 *
 *   tile 0 — transparent (game area, GB screen shows through)
 *   tile 1 — solid border colour
 *   tile 2 — solid accent colour
 *
 * SGB packet plumbing is adapted from gbdk's `sgb_border` example, which
 * sequences SGB_MASK_EN -> SGB_CHR_TRN -> SGB_PCT_TRN -> SGB_MASK_EN to
 * upload tiles, then map+palettes, then unfreeze.
 */
#include "sgb_border.h"
#include <gb/gb.h>
#include <gb/sgb.h>
#include <string.h>

#define SNES_RGB(r,g,b)     ((uint16_t)((b) << 10 | (g) << 5 | (r)))

#define SGB_BORDER_PAL       4u                  /* uses SNES palette 4 */
#define BORDER_W            32u                  /* tiles wide          */
#define BORDER_H            28u                  /* tiles tall          */
#define GAME_X               6u                  /* game area origin    */
#define GAME_Y               5u
#define GAME_W              20u
#define GAME_H              18u

/*
 * SNES tile = 4bpp planar, 32 bytes.
 *   bytes 0..15 : plane 0/1 interleaved (row 0 lo, row 0 hi, row 1 lo, ...)
 *   bytes 16..31: plane 2/3 interleaved
 *
 * Solid color N: each row's plane bit = bit n of N.
 *   color 1 (0001): plane 0 = 0xFF, planes 1/2/3 = 0x00
 *   color 2 (0010): plane 1 = 0xFF, planes 0/2/3 = 0x00
 */
#define ROW_P01_C1   0xFFu, 0x00u
#define ROW_P23_C1   0x00u, 0x00u
#define ROW_P01_C2   0x00u, 0xFFu
#define ROW_P23_C2   0x00u, 0x00u

static const uint8_t border_tiles[3 * 32] = {
    /* tile 0 — transparent (all zeros) */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* tile 1 — solid color 1 (border body) */
    ROW_P01_C1, ROW_P01_C1, ROW_P01_C1, ROW_P01_C1,
    ROW_P01_C1, ROW_P01_C1, ROW_P01_C1, ROW_P01_C1,
    ROW_P23_C1, ROW_P23_C1, ROW_P23_C1, ROW_P23_C1,
    ROW_P23_C1, ROW_P23_C1, ROW_P23_C1, ROW_P23_C1,
    /* tile 2 — solid color 2 (accent line) */
    ROW_P01_C2, ROW_P01_C2, ROW_P01_C2, ROW_P01_C2,
    ROW_P01_C2, ROW_P01_C2, ROW_P01_C2, ROW_P01_C2,
    ROW_P23_C2, ROW_P23_C2, ROW_P23_C2, ROW_P23_C2,
    ROW_P23_C2, ROW_P23_C2, ROW_P23_C2, ROW_P23_C2,
};

/* Tilemap entry packing helpers */
#define MAP_BORDER  ((uint16_t)((SGB_BORDER_PAL << 10) | 1))   /* tile 1 */
#define MAP_ACCENT  ((uint16_t)((SGB_BORDER_PAL << 10) | 2))   /* tile 2 */
#define MAP_GAME    ((uint16_t)((SGB_BORDER_PAL << 10) | 0))   /* tile 0 */

/* Generate the 32x28 tile map at startup into RAM (saves ROM repetition) */
static uint16_t border_map[BORDER_W * BORDER_H];

static void build_border_map(void) {
    uint16_t i;
    for (i = 0; i < BORDER_W * BORDER_H; i++) {
        uint8_t row = (uint8_t)(i / BORDER_W);
        uint8_t col = (uint8_t)(i % BORDER_W);

        uint8_t in_game = (row >= GAME_Y && row < GAME_Y + GAME_H &&
                           col >= GAME_X && col < GAME_X + GAME_W);

        /* Accent line: 1 tile away from the game area on all sides */
        uint8_t accent =
            (!in_game) &&
            ( (row == GAME_Y - 1u && col >= GAME_X - 1u && col <= GAME_X + GAME_W) ||
              (row == GAME_Y + GAME_H && col >= GAME_X - 1u && col <= GAME_X + GAME_W) ||
              (col == GAME_X - 1u && row >= GAME_Y - 1u && row <= GAME_Y + GAME_H) ||
              (col == GAME_X + GAME_W && row >= GAME_Y - 1u && row <= GAME_Y + GAME_H) );

        border_map[i] = in_game ? MAP_GAME : (accent ? MAP_ACCENT : MAP_BORDER);
    }
}

/*
 * SNES border palettes 4..7. We only use palette 4. Each palette is 16
 * colours x 2 bytes BGR15 = 32 bytes; total 128 bytes.
 *
 * Palette 4 colour 0 must equal palette 0 colour 0 (the SGB treats this
 * as the "transparent" colour wherever it appears in the border, letting
 * the GB screen show through). We leave it 0.
 */
static const uint16_t border_palettes[64] = {
    /* palette 4 */
    0,                          /* 0 — transparent (game area)       */
    SNES_RGB( 4,  6, 18),       /* 1 — deep navy border              */
    SNES_RGB(10, 28, 14),       /* 2 — phosphor-green accent         */
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* palettes 5, 6, 7 — unused, leave at zero */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/* ---------------------------------------------------------------------- */
/* Adapted from gbdk's set_sgb_border() helper. Inlined so we don't pull  */
/* in the example file directly.                                          */
/* ---------------------------------------------------------------------- */

#define SGB_TRANSFER(A,B)  do {        \
        map_buf[0] = (A); map_buf[1] = (B); sgb_transfer(map_buf); \
    } while (0)

static void do_set_sgb_border(const uint8_t *tiledata, uint16_t tiledata_size,
                                const uint8_t *tilemap,  uint16_t tilemap_size,
                                const uint8_t *palette,  uint16_t palette_size) {
    uint8_t map_buf[20];
    uint8_t tmp_lcdc;
    uint8_t i, x, y;
    uint8_t ntiles;

    if (!sgb_check()) return;

    memset(map_buf, 0, sizeof(map_buf));

    /* Freeze the SGB display while we abuse VRAM */
    SGB_TRANSFER((SGB_MASK_EN << 3) | 1, 1u /* freeze */);

    BGP_REG = OBP0_REG = OBP1_REG = 0xE4u;
    SCX_REG = SCY_REG = 0u;
    tmp_lcdc = LCDC_REG;

    HIDE_SPRITES; HIDE_WIN; SHOW_BKG;
    DISPLAY_ON;

    /* Lay out 256 background tiles in a 20x14 grid for SGB to capture */
    i = 0;
    for (y = 0; y < 14u; y++) {
        uint8_t *dout = map_buf;
        for (x = 0; x < 20u; x++) *dout++ = i++;
        set_bkg_tiles(0, y, 20, 1, map_buf);
    }
    memset(map_buf, 0, sizeof(map_buf));

    /* Upload tile graphics — one CHR_TRN packet per 128 tiles (4KB) */
    ntiles = (tiledata_size > (256u * 32u)) ? 0u : (uint8_t)(tiledata_size >> 5);
    if (ntiles == 0u || ntiles > 128u) {
        set_bkg_data(0, 0, (uint8_t *)tiledata);
        SGB_TRANSFER((SGB_CHR_TRN << 3) | 1, 0u);
        if (ntiles) ntiles = (uint8_t)(ntiles - 128u);
        tiledata += (128u * 32u);
        set_bkg_data(0, (uint8_t)(ntiles << 1), (uint8_t *)tiledata);
        SGB_TRANSFER((SGB_CHR_TRN << 3) | 1, 1u);
    } else {
        set_bkg_data(0, (uint8_t)(ntiles << 1), (uint8_t *)tiledata);
        SGB_TRANSFER((SGB_CHR_TRN << 3) | 1, 0u);
    }

    /* Upload tilemap + palettes via a PCT_TRN packet */
    set_bkg_data(0,   (uint8_t)(tilemap_size >> 4), (uint8_t *)tilemap);
    set_bkg_data(128, (uint8_t)(palette_size >> 4), (uint8_t *)palette);
    SGB_TRANSFER((SGB_PCT_TRN << 3) | 1, 0u);

    LCDC_REG = tmp_lcdc;

    /* Wipe the BG so the user doesn't see our scratch tiles for a frame */
    fill_bkg_rect(0, 0, 20, 18, 0);

    /* Unfreeze */
    SGB_TRANSFER((SGB_MASK_EN << 3) | 1, 0u);
}

void sgb_border_install(void) {
    if (!sgb_check()) return;
    build_border_map();
    do_set_sgb_border(border_tiles,        sizeof(border_tiles),
                      (const uint8_t *)border_map, sizeof(border_map),
                      (const uint8_t *)border_palettes, sizeof(border_palettes));
}
