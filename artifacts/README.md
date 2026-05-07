# Prebuilt ROMs

| File | Target hardware | Build flags |
|---|---|---|
| `totp-gb.gb` | DMG, Pocket, Light, Super Game Boy (DMG mode) | MBC3+TIMER+RAM+BATTERY, SGB-aware |
| `totp-gbc.gbc` | Game Boy Color, Game Boy Advance (compat mode), SGB | CGB-compatible (`0x143 = 0x80`), SGB-aware |

## GBA compatibility note

`totp-gbc.gbc` runs on a Game Boy Advance via the GBA's hardware
backward-compat mode (the GB/GBC silicon on the SoC takes over
when the cart pinout is detected). Works on GBA, GBA SP, and
original DS slot-2 — **not** GBA Micro or DS Lite.

A native GBA port (240×160 layout, 32-bit ARM7) lives in a
separate sibling repository: `totp-gba/`.
