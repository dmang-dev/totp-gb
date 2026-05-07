#include "ui.h"
#include "input.h"
#include "totp.h"
#include "storage.h"
#include "audio.h"
#include <gb/gb.h>
#include <gbdk/console.h>
#include <stdio.h>
#include <string.h>

/* Defined in main.c — current Unix timestamp for live-updating screens */
extern uint32_t current_unix_time(void);

/* ----------------------------------------------------------------------
 * Drawing primitives.
 *
 * We avoid putchar/printf for redrawn UI because GBDK's console auto-
 * scrolls when the cursor passes the bottom-right cell, which causes
 * visible flicker/scroll. setchar() writes a tile at the current cursor
 * without advancing it, so we manage position ourselves.
 * ---------------------------------------------------------------------- */

/* Write a single char at (x,y) without advancing the cursor. */
static void put_at(uint8_t x, uint8_t y, char c) {
    gotoxy(x, y);
    setchar(c);
}

/* Write w characters of s at (x,y); pad with spaces if s is shorter. */
static void draw_at(uint8_t x, uint8_t y, const char *s, uint8_t w) {
    uint8_t i;
    for (i = 0; i < w; i++) {
        char c = (s && s[i]) ? s[i] : ' ';
        gotoxy(x + i, y);
        setchar(c);
    }
}

void ui_clear(void) {
    uint8_t y, x;
    for (y = 0; y < 18u; y++) {
        for (x = 0; x < 20u; x++) {
            gotoxy(x, y);
            setchar(' ');
        }
    }
    gotoxy(0, 0);
}

/* Legacy print_fixed — uses cursor implicitly. Caller must gotoxy first.
 * Internally uses setchar so it never triggers scroll. */
static void print_fixed(const char *s, uint8_t w) {
    uint8_t x = posx();
    uint8_t y = posy();
    draw_at(x, y, s, w);
    /* Best-effort cursor advance, clamped so we never sit at (19, 17). */
    if (x + w < 20u) gotoxy(x + w, y);
    else if (y + 1u < 18u) gotoxy(0, y + 1u);
    else gotoxy(0, 0);
}

void ui_print_code(uint32_t code) {
    uint8_t d[6], i;
    uint8_t x = posx();
    uint8_t y = posy();
    uint32_t c = code;
    for (i = 6u; i > 0u; i--) { d[i-1u] = (uint8_t)('0' + c % 10u); c /= 10u; }
    put_at(x,    y, d[0]);
    put_at(x+1u, y, d[1]);
    put_at(x+2u, y, d[2]);
    put_at(x+3u, y, ' ');
    put_at(x+4u, y, d[3]);
    put_at(x+5u, y, d[4]);
    put_at(x+6u, y, d[5]);
}

/* "XXs" with blink on odd seconds when urgent. Writes 3 chars at (x,y). */
static void draw_countdown_secs(uint8_t x, uint8_t y, uint8_t secs) {
    uint8_t urgent = (secs <= 5u && secs > 0u);
    uint8_t blink  = urgent && (secs & 1u);
    if (blink) {
        put_at(x,    y, ' ');
        put_at(x+1u, y, ' ');
    } else {
        put_at(x,    y, '0' + secs / 10u);
        put_at(x+1u, y, '0' + secs % 10u);
    }
    put_at(x+2u, y, 's');
}

/* "[bars]" of given width between brackets. Writes width+2 chars at (x,y).
 * Normal: '=' fill scaled to fraction of 30s.
 * Urgent (last 5s): one '!' per second remaining. */
static void draw_countdown_bar(uint8_t x, uint8_t y, uint8_t secs, uint8_t width) {
    uint8_t i;
    uint8_t urgent = (secs <= 5u && secs > 0u);
    uint8_t bars   = urgent ? secs
                            : (uint8_t)(((uint16_t)secs * (uint16_t)width) / 30u);
    char    fill   = urgent ? '!' : '=';

    put_at(x, y, '[');
    for (i = 0; i < width; i++) put_at(x + 1u + i, y, i < bars ? fill : '-');
    put_at(x + 1u + width, y, ']');
}

void ui_print_countdown(uint8_t secs) {
    /* Combined "XXs[15-bar]" — 20 chars total at the current cursor pos. */
    uint8_t x = posx();
    uint8_t y = posy();
    draw_countdown_secs(x, y, secs);
    draw_countdown_bar(x + 3u, y, secs, 15u);
}

/* ---- settings screen --------------------------------------------------- */

void ui_screen_settings(void) {
    uint8_t row = 0u;        /* 0 = palette, 1 = sound */
    uint8_t prev_pal   = 0xFFu;
    uint8_t prev_snd   = 0xFFu;
    uint8_t prev_row   = 0xFFu;

    ui_clear();
    gotoxy(0, 0); print_fixed("    == SETTINGS ==  ", 20u);
    gotoxy(0, 1); print_fixed("--------------------", 20u);
    gotoxy(0, 14); print_fixed("--------------------", 20u);
    gotoxy(0, 15); print_fixed(" U/D:Field          ", 20u);
    gotoxy(0, 16); print_fixed(" L/R:Change         ", 20u);
    gotoxy(0, 17); print_fixed(" B/SELECT:Back      ", 20u);

    for (;;) {
        uint8_t pal = storage_get_palette();
        uint8_t snd = storage_get_sound_enabled();
        if (pal >= ui_get_palette_count()) pal = 0u;

        if (pal != prev_pal || row != prev_row) {
            put_at(1, 4, row == 0u ? '>' : ' ');
            gotoxy(2, 4); print_fixed("Palette:    ", 12u);
            put_at(15u, 4, ' ');
            put_at(16u, 4, '0' + pal / 10u);
            put_at(17u, 4, '0' + pal % 10u);
            put_at(18u, 4, ' ');
            prev_pal = pal;
        }
        if (snd != prev_snd || row != prev_row) {
            put_at(1, 7, row == 1u ? '>' : ' ');
            gotoxy(2, 7); print_fixed("Sound:      ", 12u);
            gotoxy(15, 7); print_fixed(snd ? "ON " : "OFF", 3u);
            prev_snd = snd;
        }
        prev_row = row;

        wait_vbl_done();
        input_update();

        if (input_pressed(J_DOWN) && row < 1u) { row++; sfx_click(); }
        if (input_pressed(J_UP)   && row > 0u) { row--; sfx_click(); }

        if (input_pressed(J_LEFT) || input_pressed(J_RIGHT)) {
            if (row == 0u) {
                /* cycle palette */
                uint8_t p = pal;
                if (input_pressed(J_LEFT))
                    p = (p == 0u) ? (uint8_t)(ui_get_palette_count() - 1u) : (uint8_t)(p - 1u);
                else
                    p = (uint8_t)((p + 1u) % ui_get_palette_count());
                storage_set_palette(p);
                ui_apply_palette(p);
                sfx_click();
            } else {
                /* toggle sound */
                uint8_t new_snd = !snd;
                storage_set_sound_enabled(new_snd);
                audio_set_enabled(new_snd);
                if (new_snd) sfx_confirm();
            }
        }

        if (input_pressed(J_B) || input_pressed(J_SELECT)) {
            sfx_click();
            return;
        }
    }
}

/* ---- boot splash ------------------------------------------------------- */

void ui_screen_splash(void) {
    /* Big "TOTP GB" letters drawn from the font font; animate by sliding
     * a marquee underneath that moves left to right and back. */
    static const char * const banner =
        "++++++++ +++++++ +++++++ +++++++   +++++  +++++++";
    static const char * const labels[5] = {
        "  T   O   T   P     G B  ",
        "                         ",
        "    Authenticator        ",
        "                         ",
        "                         "
    };
    uint8_t  frame;
    uint16_t banner_len;

    ui_clear();

    /* Draw centred title */
    {
        uint8_t y;
        for (y = 0; y < 5u; y++) {
            uint8_t x;
            for (x = 0; x < 20u; x++) {
                char c = labels[y][x];
                put_at(x, 5u + y, c ? c : ' ');
            }
        }
    }

    put_at(0,  12, '+');
    put_at(19, 12, '+');
    put_at(2,  14, 'P'); put_at(3, 14, 'r'); put_at(4, 14, 'e'); put_at(5, 14, 's');
    put_at(6,  14, 's'); put_at(8, 14, 'a'); put_at(9, 14, 'n'); put_at(10,14, 'y');
    put_at(12, 14, 'b'); put_at(13,14, 'u'); put_at(14,14, 't'); put_at(15,14, 't');
    put_at(16, 14, 'o'); put_at(17,14, 'n');

    banner_len = 0u;
    while (banner[banner_len]) banner_len++;

    /* Animate: scroll a "+++++++" marquee through row 13 for ~2 seconds,
     * exit early on any button press. */
    for (frame = 0u; frame < 120u; frame++) {
        uint8_t x;
        uint16_t off = (uint16_t)frame % (banner_len + 18u);
        for (x = 0; x < 18u; x++) {
            uint16_t bi;
            if (off + x < 18u || off + x - 18u >= banner_len) {
                put_at(1u + x, 13u, ' ');
            } else {
                bi = off + x - 18u;
                put_at(1u + x, 13u, banner[bi]);
            }
        }
        wait_vbl_done(); wait_vbl_done();   /* ~30fps animation */
        input_update();
        if (input_pressed(J_A) || input_pressed(J_B) ||
            input_pressed(J_START) || input_pressed(J_SELECT) ||
            input_pressed(J_UP) || input_pressed(J_DOWN) ||
            input_pressed(J_LEFT) || input_pressed(J_RIGHT))
            break;
    }
}

/* ---- main screen ------------------------------------------------------- */

#define NAME_DISPLAY_W  13u    /* visible chars in the main list */
#define SCROLL_PAD       4u    /* spaces between repetitions when looping  */
#define SCROLL_FRAMES   24u    /* frames per scroll step (~24/60 = 0.4s)   */

/* Render a name into a fixed-width slot, auto-scrolling if it doesn't fit.
 * `scroll_phase` advances 1 every SCROLL_FRAMES; we wrap modulo (len + pad). */
static void draw_scrolling_name(uint8_t x, uint8_t y, const char *name,
                                  uint8_t width, uint8_t scrolling,
                                  uint16_t scroll_phase) {
    uint8_t len = 0u;
    while (name[len] && len < 32u) len++;

    if (!scrolling || len <= width) {
        uint8_t i;
        for (i = 0; i < width; i++) {
            put_at(x + i, y, (i < len) ? name[i] : ' ');
        }
        return;
    }

    {
        uint16_t cycle  = (uint16_t)len + SCROLL_PAD;
        uint16_t offset = scroll_phase % cycle;
        uint8_t  i;
        for (i = 0; i < width; i++) {
            uint16_t pos = (offset + i) % cycle;
            put_at(x + i, y, (pos < len) ? name[(uint8_t)pos] : ' ');
        }
    }
}

void ui_draw_main(uint8_t scroll, uint8_t selected, uint32_t unix_time) {
    static uint16_t s_scroll_phase = 0u;
    static uint8_t  s_frame_ctr    = 0u;
    /* TOTP cache so we don't re-HMAC on every frame just to ticker-scroll */
    static uint32_t s_cache_window = 0xFFFFFFFFUL;
    static uint8_t  s_cache_scroll = 0xFFu;
    static uint32_t s_cache_codes[4];

    uint8_t count = storage_count();
    uint8_t rows  = 4u;
    uint8_t i;
    Account acct;
    uint32_t window = unix_time / 30UL;
    uint8_t  cache_valid = (window == s_cache_window && scroll == s_cache_scroll);

    if (++s_frame_ctr >= SCROLL_FRAMES) {
        s_frame_ctr = 0u;
        s_scroll_phase++;
    }

    /* Don't clear every call — overwrite same positions to avoid flicker. */
    gotoxy(0, 0); print_fixed("    == TOTP GB ==   ", 20u);

    gotoxy(0, 1);
    ui_print_countdown(totp_seconds_remaining(unix_time));

    gotoxy(0, 2); print_fixed("--------------------", 20u);

    for (i = 0; i < rows && (scroll + i) < count; i++) {
        uint8_t idx = scroll + i;
        uint8_t row = 3u + i * 3u;  /* rows 3, 6, 9, 12 */
        uint32_t code;

        storage_get(idx, &acct);
        if (cache_valid) {
            code = s_cache_codes[i];
        } else {
            code = totp_generate(acct.secret, unix_time);
            s_cache_codes[i] = code;
        }

        put_at(0, row, idx == selected ? '>' : ' ');
        /* Only the selected row ticker-scrolls; others stay truncated. */
        draw_scrolling_name(1u, row, acct.name, NAME_DISPLAY_W,
                            (idx == selected), s_scroll_phase);
        {
            uint8_t pad;
            for (pad = NAME_DISPLAY_W + 1u; pad < 18u; pad++)
                put_at(pad, row, ' ');
        }
        put_at(18u, row, idx == selected ? '<' : ' ');
        put_at(19u, row, ' ');

        gotoxy(2, row + 1u);
        if (code != 0xFFFFFFFFUL) {
            ui_print_code(code);
        } else {
            print_fixed("-- ---", 6u);
        }
    }

    if (count == 0u) {
        gotoxy(2, 5);  print_fixed("No accounts.", 12u);
        gotoxy(1, 7);  print_fixed("Start:Add  B:Sync", 17u);
    }

    gotoxy(0, 15); print_fixed("--------------------", 20u);
    gotoxy(0, 16); print_fixed(" A:View   START:Add ", 20u);
    gotoxy(0, 17); print_fixed(" B:Sync   SELECT:Cfg", 20u);

    s_cache_window = window;
    s_cache_scroll = scroll;
}

/* ---- character dial --------------------------------------------------- */

/*
 * Scrolling "dial" character picker — fits the 20-char GB display.
 *
 * Layout (at start_row):
 *   Row +0:  "Buf: [XXXX......   ]"  input buffer
 *   Row +1:  "  < A  B [C] D  E > "  5-char sliding window
 *   Row +2:  " A:Pick B:Del Sta:OK"  hints
 *
 * Left/Right scrolls the dial; A appends the current char;
 * B deletes the last char; Start confirms; Select cancels.
 */
static const char B32_CHARS[]  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const char NAME_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789 -_.@";

static uint8_t char_picker(char *buf, uint8_t max_len,
                            const char *charset, uint8_t charset_len,
                            uint8_t start_row) {
    uint8_t cur    = 0u;
    uint8_t buf_len = (uint8_t)strlen(buf);
    uint8_t prev_cur = 0xFFu;
    uint8_t prev_buf_len = 0xFFu;

    /* Hint row is static */
    gotoxy(0, start_row + 2u);
    print_fixed(" A:Add B:Del Sta:OK ", 20u);

    for (;;) {
        uint8_t buf_disp_start;
        uint8_t need_redraw = (cur != prev_cur) || (buf_len != prev_buf_len);

        if (!need_redraw) goto skip_draw;

        /* ---- draw buffer row ---- */
        put_at(0, start_row, '[');
        buf_disp_start = (buf_len > 17u) ? (uint8_t)(buf_len - 17u) : 0u;
        {
            uint8_t i;
            for (i = 0; i < 18u; i++) {
                uint8_t bi = buf_disp_start + i;
                put_at(1u + i, start_row,
                       bi < buf_len ? (char)buf[bi] : '_');
            }
        }
        put_at(19u, start_row, ']');

        /* ---- draw dial row: 5 chars centered on cur, slot = 4 cols ---- */
        {
            int8_t offset;
            for (offset = -2; offset <= 2; offset++) {
                int16_t ci = (int16_t)cur + offset;
                char ch;
                uint8_t col = (uint8_t)((offset + 2) * 4);
                if (ci < 0) ci += charset_len;
                if (ci >= (int16_t)charset_len) ci -= charset_len;
                ch = charset[(uint8_t)ci];
                if (offset == 0) {
                    put_at(col,      start_row + 1u, '[');
                    put_at(col + 1u, start_row + 1u, ch);
                    put_at(col + 2u, start_row + 1u, ']');
                    put_at(col + 3u, start_row + 1u, ' ');
                } else {
                    put_at(col,      start_row + 1u, ' ');
                    put_at(col + 1u, start_row + 1u, ch);
                    put_at(col + 2u, start_row + 1u, ' ');
                    put_at(col + 3u, start_row + 1u, ' ');
                }
            }
        }

        prev_cur = cur;
        prev_buf_len = buf_len;

skip_draw:
        wait_vbl_done();
        input_update();

        if (input_pressed(J_LEFT)) {
            cur = (cur == 0u) ? (uint8_t)(charset_len - 1u) : (uint8_t)(cur - 1u);
        }
        if (input_pressed(J_RIGHT)) {
            cur = (uint8_t)((cur + 1u >= charset_len) ? 0u : cur + 1u);
        }
        /* Up/Down jumps 8 chars at a time for quicker navigation */
        if (input_pressed(J_UP)) {
            cur = (cur < 8u) ? (uint8_t)(charset_len - 8u + cur) : (uint8_t)(cur - 8u);
        }
        if (input_pressed(J_DOWN)) {
            cur = (uint8_t)((cur + 8u) % charset_len);
        }

        if (input_pressed(J_A)) {
            if (buf_len < max_len - 1u) {
                buf[buf_len++] = charset[cur];
                buf[buf_len]   = '\0';
            }
        }
        if (input_pressed(J_B)) {
            if (buf_len > 0u) buf[--buf_len] = '\0';
        }
        if (input_pressed(J_START)) { return 1u; }
        if (input_pressed(J_SELECT)) { return 0u; }
    }
}

/* ---- add account screen ----------------------------------------------- */

uint8_t ui_screen_add(void) {
    Account acct;
    uint8_t confirmed;

    memset(&acct, 0, sizeof(acct));

    /* Enter name */
    do {
        ui_clear();
        gotoxy(0, 0); print_fixed("== ADD: Name ==     ", 20u);
        gotoxy(0, 1); print_fixed("Enter account label:", 20u);
        gotoxy(0, 2); print_fixed("(Start when done)   ", 20u);
        gotoxy(0, 3); print_fixed("--------------------", 20u);

        confirmed = char_picker(acct.name, ACCOUNT_NAME_LEN, NAME_CHARS,
                                (uint8_t)(sizeof(NAME_CHARS) - 1u), 4u);
        if (!confirmed) return 0u;
    } while (acct.name[0] == '\0');

    /* Enter secret */
    do {
        ui_clear();
        gotoxy(0, 0); print_fixed("== ADD: Secret ==   ", 20u);
        gotoxy(0, 1); print_fixed("Enter Base32 secret:", 20u);
        gotoxy(0, 2); print_fixed("(Start when done)   ", 20u);
        gotoxy(0, 3); print_fixed("--------------------", 20u);

        confirmed = char_picker(acct.secret, SECRET_B32_LEN, B32_CHARS,
                                (uint8_t)(sizeof(B32_CHARS) - 1u), 4u);
        if (!confirmed) return 0u;
    } while (acct.secret[0] == '\0');

    if (!storage_add(&acct)) {
        ui_clear();
        gotoxy(2, 7); print_fixed("Accounts full!", 14u);
        gotoxy(2, 8); print_fixed("Delete one first.", 17u);
        input_wait_any();
        return 0u;
    }
    return 1u;
}

/* ---- time set screen -------------------------------------------------- */

static uint8_t days_in_month(uint8_t m, uint16_t y) {
    static const uint8_t dom[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2u && y % 4u == 0u && (y % 100u != 0u || y % 400u == 0u)) return 29u;
    return dom[m - 1u];
}

static uint32_t ymd_to_epoch(uint16_t yr, uint8_t mo, uint8_t dy,
                              uint8_t hr, uint8_t mn, uint8_t sc) {
    uint32_t days = 0;
    uint16_t y; uint8_t m;
    for (y = 1970u; y < yr; y++)
        days += (y % 4u == 0u && (y % 100u != 0u || y % 400u == 0u)) ? 366u : 365u;
    for (m = 1u; m < mo; m++) days += days_in_month(m, yr);
    days += (uint32_t)(dy - 1u);
    return days * 86400UL + (uint32_t)hr * 3600UL + (uint32_t)mn * 60UL + sc;
}

/* Inverse of ymd_to_epoch — break a Unix timestamp into Y/M/D H:M:S (UTC). */
static void epoch_to_ymd(uint32_t epoch,
                          uint16_t *yr, uint8_t *mo, uint8_t *dy,
                          uint8_t *hr, uint8_t *mn, uint8_t *sc) {
    uint32_t days = epoch / 86400UL;
    uint32_t tod  = epoch % 86400UL;
    uint16_t y = 1970u;
    uint8_t  m = 1u;
    uint16_t ydays;

    *hr = (uint8_t)(tod / 3600UL);
    *mn = (uint8_t)((tod % 3600UL) / 60UL);
    *sc = (uint8_t)(tod % 60UL);

    for (;;) {
        ydays = (y % 4u == 0u && (y % 100u != 0u || y % 400u == 0u)) ? 366u : 365u;
        if (days < (uint32_t)ydays) break;
        days -= ydays;
        y++;
    }
    while (m <= 12u) {
        uint8_t dim = days_in_month(m, y);
        if (days < (uint32_t)dim) break;
        days -= dim;
        m++;
    }
    *yr = y;
    *mo = m;
    *dy = (uint8_t)(days + 1u);
}

uint32_t ui_screen_timeset(uint32_t current_epoch) {
    uint16_t yr;
    uint8_t  mo, dy, hr, mn, sc;
    uint8_t  field = 0u;
    static const char * const labels[6] = {"Year ","Month","Day  ","Hour ","Min  ","Sec  "};

    /* Pre-populate from saved epoch (or 2024-01-01 if never set) */
    if (current_epoch == 0UL) {
        yr = 2024u; mo = 1u; dy = 1u; hr = 0u; mn = 0u; sc = 0u;
    } else {
        epoch_to_ymd(current_epoch, &yr, &mo, &dy, &hr, &mn, &sc);
    }

    /* Draw static parts ONCE on entry to avoid flicker */
    ui_clear();
    gotoxy(0, 0); print_fixed("== SET TIME ==      ", 20u);
    gotoxy(0, 1); print_fixed("U/D:field  L/R:val  ", 20u);
    gotoxy(0, 2); print_fixed("Start:Save Sel:Abort", 20u);
    gotoxy(0, 3); print_fixed("--------------------", 20u);

    for (;;) {
        uint8_t i;
        uint8_t maxdy = days_in_month(mo, yr);
        if (dy > maxdy) dy = maxdy;

        /* Only redraw the value rows — the static rows above persist */
        for (i = 0; i < 6u; i++) {
            uint16_t v = 0;
            gotoxy(1, 4u + i);
            putchar(i == field ? '>' : ' ');
            print_fixed(labels[i], 5u);
            putchar(':');
            putchar(' ');
            switch (i) {
                case 0: v = yr; break; case 1: v = mo; break;
                case 2: v = dy; break; case 3: v = hr; break;
                case 4: v = mn; break; case 5: v = sc; break;
            }
            if (i == 0u) {
                putchar('0' + (uint8_t)(v / 1000u));
                putchar('0' + (uint8_t)((v / 100u) % 10u));
                putchar('0' + (uint8_t)((v / 10u) % 10u));
                putchar('0' + (uint8_t)(v % 10u));
            } else {
                putchar('0' + (uint8_t)(v / 10u));
                putchar('0' + (uint8_t)(v % 10u));
            }
        }

        wait_vbl_done();
        input_update();

        if (input_pressed(J_DOWN) && field < 5u) field++;
        if (input_pressed(J_UP)   && field > 0u) field--;

        if (input_pressed(J_RIGHT)) {
            switch (field) {
                case 0: if (yr < 2099u) yr++; break;
                case 1: mo = (mo < 12u) ? mo + 1u : 1u; break;
                case 2: dy = (dy < maxdy) ? dy + 1u : 1u; break;
                case 3: hr = (hr < 23u) ? hr + 1u : 0u; break;
                case 4: mn = (mn < 59u) ? mn + 1u : 0u; break;
                case 5: sc = (sc < 59u) ? sc + 1u : 0u; break;
            }
        }
        if (input_pressed(J_LEFT)) {
            switch (field) {
                case 0: if (yr > 1970u) yr--; break;
                case 1: mo = (mo > 1u) ? mo - 1u : 12u; break;
                case 2: dy = (dy > 1u) ? dy - 1u : maxdy; break;
                case 3: hr = (hr > 0u) ? hr - 1u : 23u; break;
                case 4: mn = (mn > 0u) ? mn - 1u : 59u; break;
                case 5: sc = (sc > 0u) ? sc - 1u : 59u; break;
            }
        }
        if (input_pressed(J_START))  return ymd_to_epoch(yr, mo, dy, hr, mn, sc);
        if (input_pressed(J_SELECT)) return current_epoch;
    }
}

/* ---- view / delete screen --------------------------------------------- */

uint8_t ui_screen_view(uint8_t idx, uint32_t unix_time) {
    Account acct;
    uint32_t code;
    uint32_t prev_window = 0xFFFFFFFFUL;
    uint8_t  prev_secs   = 0xFFu;
    uint8_t  prev_choice = 0xFFu;
    uint8_t choice = 0u;
    (void)unix_time;  /* fetched fresh each iteration */

    storage_get(idx, &acct);

    /* Static layout — drawn once */
    ui_clear();
    gotoxy(0, 0);  print_fixed(" == VIEW ACCOUNT == ", 20u);
    gotoxy(0, 1);  print_fixed("--------------------", 20u);
    gotoxy(1, 2);  print_fixed("Name:  ", 7u); print_fixed(acct.name, 13u);
    gotoxy(1, 4);  print_fixed("Code:  ", 7u);
    gotoxy(1, 5);  print_fixed("Time:  ", 7u);   /* seconds rendered live  */
    /* Countdown bar on row 6, indented 1 col on each side (width = 16) */
    gotoxy(1, 8);  print_fixed("Secret:", 7u);
    /*
     * Secret display: indented 1 col to align with the other labels.
     * Wraps over 2 rows: row 9 = chars 0-18 (19 chars), row 10 = chars 19-30
     * (12 chars). Together that's 31 chars, matching SECRET_B32_LEN-1.
     */
    {
        uint8_t i;
        for (i = 0; i < 19u; i++) {
            put_at(i + 1u, 9u,
                   (i < SECRET_B32_LEN && acct.secret[i]) ? acct.secret[i] : ' ');
        }
        for (i = 0; i < 12u; i++) {
            uint8_t bi = 19u + i;
            put_at(i + 1u, 10u,
                   (bi < SECRET_B32_LEN && acct.secret[bi]) ? acct.secret[bi] : ' ');
        }
    }
    gotoxy(0, 11); print_fixed("--------------------", 20u);
    gotoxy(0, 14); print_fixed(" A:OK      L/R:Pick ", 20u);
    gotoxy(0, 15); print_fixed(" B:Back  SELECT:Back", 20u);

    for (;;) {
        uint32_t now    = current_unix_time();
        uint32_t window = now / 30UL;
        uint8_t  secs   = totp_seconds_remaining(now);

        /* Recompute TOTP only when the 30s window rolls over */
        if (window != prev_window) {
            code = totp_generate(acct.secret, now);
            gotoxy(8, 4);
            if (code != 0xFFFFFFFFUL) ui_print_code(code);
            else                       print_fixed("ERROR  ", 7u);
            prev_window = window;
        }
        if (secs != prev_secs) {
            draw_countdown_secs(8u, 5u, secs);          /* "XXs" on Time: row */
            draw_countdown_bar (1u, 6u, secs, 16u);     /* padded bar below   */
            prev_secs = secs;
        }
        if (choice != prev_choice) {
            gotoxy(0, 12);
            if (choice == 0u) print_fixed("> Back     Delete   ", 20u);
            else              print_fixed("  Back    >Delete<  ", 20u);
            prev_choice = choice;
        }

        wait_vbl_done();
        input_update();

        if (input_pressed(J_LEFT))  choice = 0u;
        if (input_pressed(J_RIGHT)) choice = 1u;
        if (input_pressed(J_B) || input_pressed(J_SELECT)) return 0u;
        if (input_pressed(J_A)) {
            if (choice == 1u) { storage_remove(idx); return 1u; }
            return 0u;
        }
    }
}
