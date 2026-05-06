#include "input.h"

uint8_t inp_pressed;
uint8_t inp_held;

static uint8_t prev_held;

void input_update(void) {
    inp_held    = joypad();
    inp_pressed = inp_held & ~prev_held;
    prev_held   = inp_held;
}

uint8_t input_wait_any(void) {
    uint8_t k;
    /* Wait for release first to avoid spurious triggers */
    while (joypad()) { wait_vbl_done(); }
    do {
        wait_vbl_done();
        k = joypad();
    } while (!k);
    return k;
}
