#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "storage.h"

/* Screen IDs */
#define SCR_MAIN    0u
#define SCR_ADD     1u
#define SCR_TIMESET 2u
#define SCR_VIEW    3u
#define SCR_DELETE  4u

/* Draw the full main list screen */
void ui_draw_main(uint8_t scroll, uint8_t selected, uint32_t unix_time);

/* Draw the Add Account screen; returns 1 if account was saved, 0 if cancelled */
uint8_t ui_screen_add(void);

/* Draw the time-set screen; returns the unix timestamp the user entered */
uint32_t ui_screen_timeset(uint32_t current_epoch);

/* Draw the View/Delete screen for one account; returns 1 if user deleted it */
uint8_t ui_screen_view(uint8_t idx, uint32_t unix_time);

/* Print a 6-digit code as "DDD DDD" at current cursor position */
void ui_print_code(uint32_t code);

/* Print a countdown bar (0-30) as a progress-style indicator */
void ui_print_countdown(uint8_t secs);

/* Clear the screen (fill background with spaces) */
void ui_clear(void);

/* Animated boot splash. Blocks until any button or 2 seconds. */
void ui_screen_splash(void);

#endif
