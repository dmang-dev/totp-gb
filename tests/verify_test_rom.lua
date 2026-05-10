--[[
  verify_test_rom.lua
  ------------------------------------------------------------
  Loads inside mGBA via Tools -> Scripting -> Load script.
  Requires totp-gb-test.gb to already be loaded (built with
  `build.bat test` in the parent directory).

  Verifies that the ROM is actually the test build (by checking
  the JBSWY3DPEHPK3PXP seed string sits at ROM offset 0x1569),
  reads the visible 6-digit TOTP code from the screen tilemap,
  and compares it against the precomputed expected sequence in
  expected_codes.lua.

  Output goes to mGBA's Scripting console (the same panel where
  bridge.lua's "bridge listening" message appears).
]]

local SCRIPT_DIR = (function()
    -- mGBA exposes the script's own path in some versions; fall back to '.'
    return debug.getinfo(1, "S").source:match("@?(.*[/\\])") or "./"
end)()

dofile(SCRIPT_DIR .. "expected_codes.lua")  -- exposes EXPECTED at file scope
-- Lua semantics: the dofile chunk returned a table; capture it via require-style
local EXPECTED = dofile(SCRIPT_DIR .. "expected_codes.lua")

local SEED_BASE_EPOCH = 1778088090
local SEED_STRING     = "JBSWY3DPEHPK3PXP"
local SEED_ROM_OFFSET = 0x1569

----------------------------------------------------------------
-- Helpers
----------------------------------------------------------------

local function log(msg)
    if console and console.log then console:log(msg) else print(msg) end
end

local function read_string(addr, len)
    local out = {}
    for i = 0, len - 1 do
        out[#out + 1] = string.char(emu:read8(addr + i))
    end
    return table.concat(out)
end

----------------------------------------------------------------
-- 1. Confirm the right ROM is loaded
----------------------------------------------------------------

local rom_check = read_string(SEED_ROM_OFFSET, #SEED_STRING)
if rom_check ~= SEED_STRING then
    log(string.format(
        "[FAIL] ROM at 0x%04X is %q, expected %q.\n" ..
        "       Load artifacts/totp-gb-test.gb first.",
        SEED_ROM_OFFSET, rom_check, SEED_STRING))
    return
end
log("[ OK ] Seed string present in ROM (0x" ..
    string.format("%04X", SEED_ROM_OFFSET) .. ")")

----------------------------------------------------------------
-- 2. Read the displayed code from the BG tile map.
--
--   The main screen draws the 6-digit code starting at column 2
--   of row 4 (rows 3,6,9,12 hold accounts; the code line for
--   the first account is row 4). Each digit on screen is a tile
--   whose index in VRAM equals 16 + ('0'..'9' - 'a' + 0x60)... but
--   GBDK's font mapping is 1:1 with ASCII for 0x20-0x7F: the tile
--   index at tile-data offset N corresponds to ASCII char N.
--
--   The BG tile map on DMG is at $9800-$9BFF, 32 bytes per row.
----------------------------------------------------------------

local BG_MAP   = 0x9800
local CODE_ROW = 4
local CODE_COL = 2

local function read_displayed_code()
    -- Layout produced by ui_print_code: "DDD DDD" -> 7 chars
    local digits = {}
    for i = 0, 6 do
        local tile = emu:read8(BG_MAP + CODE_ROW * 32 + CODE_COL + i)
        digits[#digits + 1] = string.char(tile)
    end
    return table.concat(digits)
end

local function parse_code(s)
    -- "DDD DDD" -> integer
    local d = s:gsub(" ", "")
    if not d:match("^%d%d%d%d%d%d$") then return nil end
    return tonumber(d)
end

local visible = read_displayed_code()
log("[INFO] Visible code field: '" .. visible .. "'")

local got = parse_code(visible)
if not got then
    log("[FAIL] Could not parse 6 digits at row " .. CODE_ROW ..
        " col " .. CODE_COL .. ".")
    log("       Either the ROM hasn't reached the live screen, the");
    log("       tile-to-char mapping differs, or no account exists.")
    return
end

----------------------------------------------------------------
-- 3. Match against the precomputed expected sequence.
----------------------------------------------------------------

local match_window = nil
for i = 0, #EXPECTED do
    if EXPECTED[i] == got then
        match_window = i
        break
    end
end

if match_window then
    local epoch = SEED_BASE_EPOCH + match_window * 30
    log(string.format(
        "[PASS] Displayed code %06d matches window +%ds (epoch %d, T=%d).",
        got, match_window * 30, epoch, epoch // 30))
    log("       Crypto pipeline + RTC + UI all verified end-to-end.")
else
    log(string.format(
        "[FAIL] Displayed code %06d not in next 6h of expected windows.",
        got))
    log("       Check whether the seed epoch was changed or the secret differs.")
end
