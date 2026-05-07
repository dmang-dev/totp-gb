# Prebuilt ROMs

| File | Target hardware | Build flags |
|---|---|---|
| `totp-gb.gb` | DMG, Pocket, Light, Super Game Boy (DMG mode) | MBC3+TIMER+RAM+BATTERY, SGB-aware |
| `totp-gbc.gbc` | Game Boy Color, Game Boy Advance (compat mode), SGB | CGB-compatible (`0x143 = 0x80`), SGB-aware |
| `totp-gba-test.gbc` | **Same binary as `totp-gbc.gbc`**, copy intended for GBA hardware testing | — |

## GBA compatibility note

The Game Boy Advance has GB/GBC backward-compat hardware on the
SoC and runs the GBC silicon when it detects a GB-shaped cart in
the slot. So `totp-gba-test.gbc` is byte-identical to
`totp-gbc.gbc`; the separate name just keeps the testing intent
visible.

To try it on real hardware:

1. Flash `totp-gba-test.gbc` onto a GB/GBC flashcart with
   battery-backed SRAM (EZ-FLASH Junior, EverDrive GB, or any
   MBC3+RAM+BATTERY clone cart).
2. Plug into a GBA, GBA SP, or original DS (which inherit the
   backward-compat hardware). **Won't work on GBA Micro or
   DS Lite** — Nintendo dropped the GB silicon there.
3. Power on. The phosphor-green palette uses the GBC code path
   automatically.

## Native GBA build

A real GBA port (240×160, 32-bit ARM7, native colour palettes,
GBA-shaped UI) lives in `../totp-gba/` once that project is
underway.
