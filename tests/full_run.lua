-- Full playthrough on the DEBUG ROM: title -> stage 1 -> boss -> clear -> stage 2 -> ... -> final boss
-- -> ending -> title. Uses the debug test hooks (invincible + autofire) and a simple bot that
-- follows the boss vertically, so the run is deterministic and always completes.

local function bot()
    local st = tel("state")
    if st ~= STATE.PLAYING then
        key_up(KEY.UP); key_up(KEY.DOWN); key_up(KEY.LEFT); key_up(KEY.RIGHT)
        return
    end
    local py, px = tel("player_y"), tel("player_x")
    local ty
    if tel("boss_active") == 1 then
        ty = tel("boss_y")
    else
        ty = math.floor(40 * math.sin(tel("stage_frame") / 70))
    end
    if py < ty - 2 then key_down(KEY.DOWN); key_up(KEY.UP)
    elseif py > ty + 2 then key_down(KEY.UP); key_up(KEY.DOWN)
    else key_up(KEY.UP); key_up(KEY.DOWN) end
    if px < -75 then key_down(KEY.RIGHT); key_up(KEY.LEFT)
    elseif px > -65 then key_down(KEY.LEFT); key_up(KEY.RIGHT)
    else key_up(KEY.LEFT); key_up(KEY.RIGHT) end
end

local stats = { cpu_max = 0, sprites_max = 0, bullets_max = 0, enemies_max = 0, shots_max = 0,
                cpu_sum = 0, cpu_frames = 0, obj_tiles_max = 0, bg_tiles_max = 0, map_cells_max = 0,
                powerups_max = 0, power_max = 0, shooters_max = 0, shield_max = 0, shockwaves = 0 }

local heavy_logged = 0
local prof_sum, prof_max = {}, {}
for i = 0, 11 do prof_sum[i] = 0; prof_max[i] = 0 end

-- cpu_pct is bn::core::last_cpu_usage(), i.e. the PREVIOUS frame, so heavy frames are reported with
-- the previous frame's per-system profile.
local prev_prof, prev_app = {}, 0

local function track()
    if tel("state") == STATE.PLAYING and tel("stage_frame") > 3 then
        local cpu = tel("cpu_pct")
        if cpu >= 80 and heavy_logged < 40 then
            heavy_logged = heavy_logged + 1
            local parts = {}
            for i = 0, 11 do
                local v = prev_prof[i] or 0
                if v >= 20 then parts[#parts + 1] = string.format("%s %.1f", PROF_NAMES[i], v / 10) end
            end
            log(string.format("heavy frame: cpu=%d%% %s | app %.1f: %s", cpu, snapshot_line(), prev_app / 10,
                table.concat(parts, ", ")))
        end
        stats.cpu_max = math.max(stats.cpu_max, cpu)
        stats.cpu_sum = stats.cpu_sum + cpu
        stats.cpu_frames = stats.cpu_frames + 1
        for i = 0, 11 do
            local v = tel("prof" .. i)
            prof_sum[i] = prof_sum[i] + v
            prof_max[i] = math.max(prof_max[i], v)
            prev_prof[i] = v
        end
        prev_app = tel("prof_app")
        stats.obj_tiles_max = math.max(stats.obj_tiles_max, tel("sprite_tiles_used"))
        stats.bg_tiles_max = math.max(stats.bg_tiles_max, tel("bg_tiles_used"))
        stats.map_cells_max = math.max(stats.map_cells_max, tel("bg_map_cells_used"))
        stats.sprites_max = math.max(stats.sprites_max, tel("sprites_used"))
        stats.bullets_max = math.max(stats.bullets_max, tel("enemy_bullets"))
        stats.enemies_max = math.max(stats.enemies_max, tel("enemies"))
        stats.shots_max = math.max(stats.shots_max, tel("player_shots"))
        stats.powerups_max = math.max(stats.powerups_max, tel("powerups"))
        stats.power_max = math.max(stats.power_max, tel("power"))
        stats.shooters_max = math.max(stats.shooters_max, tel("shooters"))
        stats.shield_max = math.max(stats.shield_max, tel("shield"))
    end
end

run_test(function()
    wait(60)
    check("telemetry block found", magic_ok())
    press(KEY.START)
    wait(10)
    check("game started", tel("state") == STATE.PLAYING)

    on_frame(function()
        set_ctl("ctl_invincible", 1)
        set_ctl("ctl_autofire", 1)
    end)
    on_frame(bot)
    on_frame(track)

    for stage = 0, 2 do
        check(string.format("stage %d running", stage + 1), tel("stage") == stage and tel("state") == STATE.PLAYING,
            snapshot_line())
        wait(1500)
        shot(string.format("stage%d_a", stage + 1))
        wait(1500)
        shot(string.format("stage%d_b", stage + 1))

        local boss_seen = wait_until(function() return tel("boss_active") == 1 end, 4000, 10)
        check(string.format("stage %d boss appears", stage + 1), boss_seen, snapshot_line())
        wait(200)
        shot(string.format("stage%d_boss", stage + 1))
        local hp0 = tel("boss_hp")
        check(string.format("stage %d boss has HP", stage + 1), hp0 > 0, hp0)
        wait(400)
        check(string.format("stage %d boss takes damage", stage + 1), tel("boss_hp") < hp0,
            hp0 .. " -> " .. tel("boss_hp"))
        wait(300)
        shot(string.format("stage%d_boss_late", stage + 1))

        local cleared = wait_until(function()
            local s = tel("state")
            return s == STATE.STAGE_CLEAR or s == STATE.ENDING
        end, 9000, 10)
        check(string.format("stage %d cleared", stage + 1), cleared, snapshot_line())
        log(string.format("stage %d cleared at frame %d: %s", stage + 1, tel("frame"), snapshot_line()))
        log(string.format("stage %d power at clear: step %d (%s)", stage + 1, tel("power"), power_line()))
        wait(30)
        shot(string.format("stage%d_clear", stage + 1))

        if stage < 2 then
            local next_ok = wait_until(function()
                return tel("state") == STATE.PLAYING and tel("stage") == stage + 1
            end, 900, 5)
            check(string.format("advances to stage %d", stage + 2), next_ok, snapshot_line())
            wait(60)
            shot(string.format("stage%d_start", stage + 2))
        end
    end

    local ending = wait_until(function() return tel("state") == STATE.ENDING end, 900, 5)
    check("final boss leads to ENDING", ending, snapshot_line())
    wait(120)
    shot("ending_1")
    press(KEY.START)
    wait(120)
    shot("ending_2")
    press(KEY.START)
    wait(120)
    shot("ending_3")
    press(KEY.START)
    wait(30)
    check("ending returns to TITLE", tel("state") == STATE.TITLE, STATE_NAME[tel("state")])
    check("hi-score updated", tel("hiscore") >= tel("score") and tel("hiscore") > 20000, tel("hiscore"))
    shot("title_after")

    log(string.format("max cpu %d%%, max sprites %d, max enemy bullets %d, max enemies %d, max shots %d",
        stats.cpu_max, stats.sprites_max, stats.bullets_max, stats.enemies_max, stats.shots_max))
    log(string.format("average cpu %.1f%% over %d gameplay frames", stats.cpu_sum / math.max(1, stats.cpu_frames),
        stats.cpu_frames))
    log(string.format("VRAM peak: OBJ tiles %d/1024, BG tiles %d, BG map cells %d",
        stats.obj_tiles_max, stats.bg_tiles_max, stats.map_cells_max))
    local parts = {}
    for i = 0, 11 do
        parts[#parts + 1] = string.format("%s %.1f/%.1f", PROF_NAMES[i],
            prof_sum[i] / math.max(1, stats.cpu_frames) / 10, prof_max[i] / 10)
    end
    log("system cost % of frame (avg/max): " .. table.concat(parts, ", "))
    log(string.format("power peak: step %d, shooters %d, shield %d", stats.power_max, stats.shooters_max,
        stats.shield_max))
    check("power capsules dropped by destroyed enemies", stats.powerups_max > 0, stats.powerups_max)
    check("power ladder advanced by collecting capsules", stats.power_max >= 3, stats.power_max)
    check("OBJ VRAM within 32 KB (1024 4bpp tiles)", stats.obj_tiles_max <= 1024, stats.obj_tiles_max)
    check("CPU budget: worst frame under 100%", stats.cpu_max < 100, stats.cpu_max .. "%")
    check("no missed frames", tel("missed_frames") == 0, tel("missed_frames"))
    check("sprite budget respected (<=128)", stats.sprites_max <= 128, stats.sprites_max)
    log("final: " .. snapshot_line())
end)
