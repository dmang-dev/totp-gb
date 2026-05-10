# Screen gallery

All screenshots captured from `artifacts/totp-gb-test.gb` running
in mGBA, driven via the [mcp-mgba](https://github.com/dmang-dev/mcp-mgba)
server. Test ROM seeds a `Test` account with the `JBSWY3DPEHPK3PXP`
secret on every boot, which is why the displayed code is always
in the precomputed sequence (see `tests/expected_codes.lua`).

| Screen | Image | How to reach |
|---|---|---|
| Main list — selected account, live code, 30s bar | `01-main-list.png` | Boot |
| View account — full secret, code, delete option | `02-view-account.png` | A on main |
| Settings — palette picker, sound toggle | `03-settings.png` | SELECT on main |
| Set time — Y/M/D/h/m/s with field cursor | `04-time-set.png` | B on main |
| Add account (name) — char picker dial | `05-add-name.png` | START on main |

Welcome screen is only visible on a fresh save (no magic header
in SRAM) and so isn't reachable from the test build. Load the
plain `totp-gb.gb` against an empty `.sav` to see it.

## Regenerating

These were captured manually in this session. To re-run:

1. Load `artifacts/totp-gb-test.gb` in mGBA with the mcp-mgba
   bridge active.
2. Drive mGBA via MCP (`mgba_press_buttons` for navigation,
   `mgba_screenshot` for capture).

Future: a Lua script that does this end-to-end without an MCP
client would be nicer.
