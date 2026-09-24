-- Space Shooter automated test harness for mGBA (0.11+ / nightly, run with `mGBA --script`).
--
-- The runner (tests/run_tests.ps1) generates a wrapper that defines:
--   TELEMETRY_ADDR  address of the ss_telemetry block (from arm-none-eabi-nm)
--   OUT_DIR         directory for screenshots and the result file
--   TEST_NAME       name of the test
-- and then loads this file followed by the test script.
--
-- A test is a Lua function run as a coroutine: helpers such as wait() yield to the emulator
-- for a number of frames, so tests read like sequential scripts.

KEY = { A = 0, B = 1, SELECT = 2, START = 3, RIGHT = 4, LEFT = 5, UP = 6, DOWN = 7, R = 8, L = 9 }

-- Field offsets inside ss_telemetry_block (must match src/core/ss_telemetry.h).
local FIELDS = {
    frame = { 8, 4 }, stage_frame = { 12, 4 }, score = { 16, 4 }, hiscore = { 20, 4 },
    state = { 24, 1 }, stage = { 25, 1 }, lives = { 26, 1 }, boss_active = { 27, 1 },
    player_x = { 28, -2 }, player_y = { 30, -2 }, boss_hp = { 32, -2 }, boss_hp_max = { 34, -2 },
    boss_y = { 36, -2 }, enemies = { 38, 1 }, enemy_bullets = { 39, 1 }, player_shots = { 40, 1 },
    effects = { 41, 1 }, powerups = { 42, 1 }, weapon = { 43, 1 }, weapon_level = { 44, 1 },
    missile_level = { 45, 1 }, speed_level = { 46, 1 }, shield = { 47, 1 }, cpu_pct = { 48, 1 },
    cpu_pct_max = { 49, 1 }, missed_frames = { 50, 2 }, pool_drops = { 52, 2 }, player_alive = { 54, 1 },
    deaths = { 55, 1 }, sprites_used = { 56, 1 }, ctl_invincible = { 57, 1 }, ctl_autofire = { 58, 1 },
    ctl_skip_to_boss = { 59, 1 }, music_playing = { 60, 1 }, cpu_max_stage_frame = { 64, 4 },
    cpu_max_frame = { 68, 4 }, sprite_tiles_used = { 72, 2 }, bg_tiles_used = { 74, 2 },
    bg_map_cells_used = { 76, 2 },
}
-- prof0..prof11: per-system cost of the last gameplay frame in 1/1000 frame (test-hook builds).
for i = 0, 11 do FIELDS["prof" .. i] = { 78 + 2 * i, 2 } end
PROF_NAMES = { [0] = "stage", "player", "shots", "enemies", "boss", "bullets", "powerups", "collide",
               "effects", "terrain", "bgs+camera", "hud" }

-- GBA sound registers (GBATEK "GBA Sound Control Registers").
function sound_status()
    local cnt_h = emu:read16(0x04000082)   -- SOUNDCNT_H: Direct Sound A/B routing
    local cnt_x = emu:read16(0x04000084)   -- SOUNDCNT_X: bit 7 = master enable
    local tm0 = emu:read16(0x04000102)     -- TM0CNT_H: bit 7 = timer running (sample rate)
    return {
        master = (cnt_x & 0x80) ~= 0,
        dsound = (cnt_h & 0x3300) ~= 0,    -- A/B enabled on left/right
        timer = (tm0 & 0x80) ~= 0,
        raw = string.format("SOUNDCNT_H=%04X SOUNDCNT_X=%04X TM0CNT_H=%04X", cnt_h, cnt_x, tm0),
    }
end

STATE = { TITLE = 0, PLAYING = 1, PAUSED = 2, STAGE_CLEAR = 3, GAME_OVER = 4, ENDING = 5 }
STATE_NAME = { [0] = "TITLE", "PLAYING", "PAUSED", "STAGE_CLEAR", "GAME_OVER", "ENDING" }

function tel(name)
    local f = FIELDS[name]
    assert(f, "unknown telemetry field " .. tostring(name))
    local addr = TELEMETRY_ADDR + f[1]
    local size = math.abs(f[2])
    local v
    if size == 1 then v = emu:read8(addr)
    elseif size == 2 then v = emu:read16(addr)
    else v = emu:read32(addr) end
    if f[2] == -2 and v >= 0x8000 then v = v - 0x10000 end
    return v
end

function set_ctl(name, value)
    local f = FIELDS[name]
    emu:write8(TELEMETRY_ADDR + f[1], value)
end

function magic_ok()
    local s = ""
    for i = 0, 7 do s = s .. string.char(emu:read8(TELEMETRY_ADDR + i)) end
    return s == "SSTELEM1"
end

-- ----- input ------------------------------------------------------------------------------------
local held = 0

local function apply_keys()
    emu:setKeys(held)
end

function key_down(k) held = held | (1 << k); apply_keys() end
function key_up(k) held = held & ~(1 << k); apply_keys() end
function release_all() held = 0; apply_keys() end

-- ----- scheduling -------------------------------------------------------------------------------
local co = nil
local wait_frames = 0
local frame_hooks = {}

function wait(n)
    wait_frames = n or 1
    coroutine.yield()
end

function press(k, frames)
    key_down(k)
    wait(frames or 3)
    key_up(k)
    wait(2)
end

function hold(k, frames)
    key_down(k)
    wait(frames)
    key_up(k)
end

--- Waits until cond() is true or max_frames elapse. Returns true if the condition was met.
function wait_until(cond, max_frames, step)
    step = step or 1
    local n = 0
    while n < max_frames do
        if cond() then return true end
        wait(step)
        n = n + step
    end
    return cond()
end

--- Registers a function called every frame (e.g. a bot controlling the ship).
function on_frame(fn)
    table.insert(frame_hooks, fn)
end

function clear_frame_hooks()
    frame_hooks = {}
end

-- ----- results ----------------------------------------------------------------------------------
local results = {}
local failures = 0
local log_lines = {}

function log(msg)
    table.insert(log_lines, msg)
    console:log(msg)
end

function check(name, cond, detail)
    local ok = cond and true or false
    if not ok then failures = failures + 1 end
    table.insert(results, string.format("[%s] %s%s", ok and "PASS" or "FAIL", name,
        detail and ("  (" .. tostring(detail) .. ")") or ""))
    return ok
end

function shot(name)
    emu:screenshot(OUT_DIR .. "/" .. TEST_NAME .. "_" .. name .. ".png")
end

function snapshot_line()
    return string.format("frame=%d state=%s stage=%d sf=%d lives=%d score=%d boss=%d/%d E=%d B=%d S=%d spr=%d cpu=%d%% max=%d%% drops=%d missed=%d",
        tel("frame"), STATE_NAME[tel("state")] or "?", tel("stage"), tel("stage_frame"), tel("lives"), tel("score"),
        tel("boss_hp"), tel("boss_hp_max"), tel("enemies"), tel("enemy_bullets"), tel("player_shots"),
        tel("sprites_used"), tel("cpu_pct"), tel("cpu_pct_max"), tel("pool_drops"), tel("missed_frames"))
end

local function finish(err)
    local f = io.open(OUT_DIR .. "/" .. TEST_NAME .. "_result.txt", "w")
    if err then
        failures = failures + 1
        table.insert(results, "[FAIL] script error: " .. tostring(err))
    end
    for _, l in ipairs(results) do f:write(l, "\n") end
    f:write("--- log ---\n")
    for _, l in ipairs(log_lines) do f:write(l, "\n") end
    f:write(string.format("SUMMARY %s: %d checks, %d failed\n", TEST_NAME, #results, failures))
    f:close()
    os.exit(failures == 0 and 0 or 1)
end

function run_test(fn)
    co = coroutine.create(function()
        fn()
    end)
    callbacks:add("frame", function()
        for _, hook in ipairs(frame_hooks) do hook() end
        if wait_frames > 0 then
            wait_frames = wait_frames - 1
            if wait_frames > 0 then return end
        end
        if coroutine.status(co) == "dead" then return end
        local ok, err = coroutine.resume(co)
        if not ok then
            finish(err)
        elseif coroutine.status(co) == "dead" then
            finish(nil)
        end
    end)
end
