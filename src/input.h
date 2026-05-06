#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <gb/gb.h>

/*
 * Debounced joypad helpers.
 * Call input_update() once per frame; query with input_pressed() /
 * input_held().
 */

extern uint8_t inp_pressed;   /* buttons newly pressed this frame */
extern uint8_t inp_held;      /* buttons currently held */

void input_update(void);

/* Returns non-zero if button was newly pressed this frame */
#define input_pressed(btn) (inp_pressed & (btn))
/* Returns non-zero if button is held */
#define input_held(btn)    (inp_held    & (btn))

/*
 * Block until any button is pressed, then return which one (raw joypad mask).
 */
uint8_t input_wait_any(void);

#endif
