#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/* Initialise APU (call once at boot, after NR52 has been re-enabled). */
void audio_init(void);

/* Toggle sound on/off (persisted in SRAM byte by caller). */
void audio_set_enabled(uint8_t on);
uint8_t audio_is_enabled(void);

/* One-shot SFX. All non-blocking — fire and forget. */
void sfx_click(void);     /* high blip — navigation, palette cycle */
void sfx_confirm(void);   /* lower thump — A on a confirm screen   */
void sfx_error(void);     /* harsh buzz — invalid input            */
void sfx_flip(void);      /* short tone sweep — TOTP rolled over   */

#endif
