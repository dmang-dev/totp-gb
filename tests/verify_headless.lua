--[[
  verify_headless.lua
  ----------------------------------------------------------------
  Headless variant of verify_test_rom.lua. Same checks (ROM seed
  string + visible code from BG tile map vs. expected sequence)
  but writes PASS/FAIL into tests/result.txt instead of (or in
  addition to) console.log, so a CI runner can poll for the file
  and parse it.

  Wait ~120 frames after launch so the live screen has settled,
  then snapshot once and exit. Designed to be loaded via
      mgba --script tests/verify_headless.lua artifacts/totp-gb-test.gb
  with run_headless.sh wrapping the kill timeout.
]]

local SCRIPT_DIR = (function()
    return debug.getinfo(1, "S").source:match("@?(.*[/\\])") or "./"
end)()

local SETTLE_FRAMES   = 120
local TIMEOUT_FRAMES  = 600

local SEED_BASE_EPOCH = 1778088090
local SEED_STRING     = "JBSWY3DPEHPK3PXP"
local SEED_ROM_OFFSET = 0x1569
local BG_MAP          = 0x9800
local CODE_ROW        = 4
local CODE_COL        = 2

local EXPECTED        = dofile(SCRIPT_DIR .. "expected_codes.lua")

local function log(msg)
    if console and console.log then console:log(msg) else print(msg) end
end

local function write_result(verdict, detail)
    local f = io.open(SCRIPT_DIR .. "result.txt", "w")
    if not f then log("[err] cannot open result.txt"); return end
    f:write(verdict)
    if detail then f:write(":" .. detail) end
    f:write("\n"); f:close()
    log("[" .. verdict .. "] " .. (detail or ""))
end

local function read_string(addr, len)
    local out = {}
    for i = 0, len - 1 do
        out[#out + 1] = string.char(emu:read8(addr + i))
    end
    return table.concat(out)
end

local function read_displayed_code()
    local digits = {}
    for i = 0, 6 do
        digits[#digits + 1] = string.char(emu:read8(BG_MAP + CODE_ROW * 32 + CODE_COL + i))
    end
    return table.concat(digits)
end

local function parse_code(s)
    local d = (s or ""):gsub(" ", "")
    if not d:match("^%d%d%d%d%d%d$") then return nil end
    return tonumber(d)
end

local function check()
    if read_string(SEED_ROM_OFFSET, #SEED_STRING) ~= SEED_STRING then
        write_result("FAIL", "wrong ROM (no seed string at 0x" ..
            string.format("%04X", SEED_ROM_OFFSET) .. ")")
        return
    end
    local visible = read_displayed_code()
    local got = parse_code(visible)
    if not got then
        write_result("FAIL", "could not parse visible code '" .. visible .. "'")
        return
    end
    for i = 0, #EXPECTED do
        if EXPECTED[i] == got then
            local epoch = SEED_BASE_EPOCH + i * 30
            write_result("PASS", string.format(
                "code %06d at window +%ds (epoch %d)", got, i * 30, epoch))
            return
        end
    end
    write_result("FAIL", string.format(
        "displayed code %06d not in expected sequence", got))
end

local frame = 0
local fired = false
callbacks:add("frame", function()
    if fired then return end
    frame = frame + 1
    if frame == SETTLE_FRAMES then
        check(); fired = true
    elseif frame >= TIMEOUT_FRAMES then
        write_result("TIMEOUT", "settle never reached"); fired = true
    end
end)
log("[info] verify_headless armed; will check at frame " .. SETTLE_FRAMES)
