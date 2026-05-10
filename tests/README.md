# totp-gb verification harness

Two layers of test exist:

1. **Boot self-test** (already in every build) — `mgba_log` writes
   PASS/FAIL for 4 known-answer vectors at boot. Visible in
   mGBA's Log window. Algorithm-only; no I/O.

2. **End-to-end verification** (this directory) — needs a special
   `totp-gb-test.gb` build that pre-seeds an account in SRAM. The
   verifier checks that the live on-screen code matches the
   precomputed sequence for the seeded epoch.

This file is about #2. For #1, just open Logs and reload the ROM.

---

## What the test does

The seeded ROM (built with `build.bat test`) writes a fixture into
SRAM at every boot:

| Field   | Value |
|---------|---|
| Magic   | `0xAA 0x55` |
| Count   | `1` |
| Epoch   | `1778088090` (2026-05-06 17:21:30 UTC) |
| Account | name `Test`, secret `JBSWY3DPEHPK3PXP` ("Hello!\\xDE\\xAD\\xBE\\xEF") |

Because of the seed, the welcome and time-set screens are skipped
on every boot — the ROM goes straight to the live screen showing
the "Test" account with its current TOTP code.

The verifier:

1. Reads ROM at offset `0x1569` to confirm the seed string is
   compiled in (i.e., you have `totp-gb-test.gb`, not the plain
   `totp-gb.gb`).
2. Reads 7 tile indices from the BG tile map at row 4 col 2,
   parsing them as ASCII to get the displayed `DDD DDD` code.
3. Looks the code up in `expected_codes.lua`, a 720-entry table
   covering 6 hours of 30-second windows starting at the seed
   epoch. A hit is a PASS plus a printout of which window we're in.

The match is robust against the MBC3 RTC ticking — even if you've
left the cart powered for a while, the displayed code should still
be in the table.

---

## Files

| File | Purpose |
|---|---|
| `verify_test_rom.lua` | Lua script run inside mGBA |
| `expected_codes.lua` | 720 precomputed TOTP codes |
| `gen_expected.py` | Regenerates `expected_codes.lua` if seed changes |
| `README.md` | This file |

---

## Running the verifier

1. Build the test ROM (from the project root):
   ```
   build.bat test         # Windows
   make test              # Linux / macOS / CI
   ```
   Produces `artifacts/totp-gb-test.gb`.
2. Open mGBA. **File -> Load ROM ->** `artifacts/totp-gb-test.gb`.
   You should land on the live screen showing `Test` with a
   counting code.
3. **Tools -> Scripting -> File -> Load script ->**
   `tests/verify_test_rom.lua`.
4. Look at the Scripting console. Expected output:
   ```
   [ OK ] Seed string present in ROM (0x1569)
   [INFO] Visible code field: '813 431'
   [PASS] Displayed code 813431 matches window +120s (epoch 1778088210, T=59269607).
          Crypto pipeline + RTC + UI all verified end-to-end.
   ```

---

## Re-running after source changes

The seed code lives in `src/storage.c` behind `#ifdef
TEST_SEED_ON_BOOT`. If you change the seed (epoch, secret, name),
also regenerate the expected table:

```
python tests/gen_expected.py > tests/expected_codes.lua
```

Then rebuild and re-load.

---

## Verifying via the mGBA MCP server (alternative path)

The same checks are scriptable from outside mGBA via the
[mcp-mgba](https://github.com/dmang-dev/mcp-mgba) server, useful
when integrating with Claude Code or another MCP client. The
relevant calls:

| Step | MCP call |
|---|---|
| 1. Confirm test ROM loaded | `mgba_read_range(0x1569, 16)` -> bytes for `JBSWY3DPEHPK3PXP` |
| 2. Read displayed code | `mgba_read_range(0x9800 + 4*32 + 2, 7)` -> ASCII tile indices for the code line |
| 3. Compare | look up bytes in `expected_codes.lua` (use any TOTP library to compute on the fly) |
| 4. Screenshot | `mgba_screenshot("...")` |

Bridge limitations to be aware of (see
[mcp-mgba's `docs/GB-COMPAT-FINDINGS.md`](https://github.com/dmang-dev/mcp-mgba/blob/main/docs/GB-COMPAT-FINDINGS.md)
for details — most are fixed in v0.2.0+):

- Pre-v0.2.0: `emu:write8` did **not** trigger MBC3 register writes,
  so SRAM couldn't be seeded directly — this test build (with the
  `TEST_SEED_ON_BOOT` flag) was the workaround. v0.2.0 added
  `mgba_save_state` / `mgba_load_state` which sidestep the issue.
- Pre-v0.2.0: `mgba_pause` and `mgba_advance_frames` were nil on
  some mGBA builds; v0.2.0 feature-detects and exposes capabilities
  in `mgba_get_info`.

---

## Adding more vectors

To extend coverage (e.g. test multiple secrets or a non-default
window size), drop a new fixture into the seed block in
`src/storage.c`, regenerate the expected table, rebuild, and
update `verify_test_rom.lua` to inspect the additional
account row.
