-- Power ladder on the DEBUG ROM: every step's loadout, firing with all guns, the periodic shockwave,
-- and the reset to the normal shot when a ship is lost. The ctl_set_power test hook jumps to a step
-- (random capsule drops are covered by full_run). Starts in stage 2 so terrain can destroy the ship.

-- Expected loadout per step: main gun (0 normal, 1 laser, 2 spread laser), missiles (0 none,
-- 1 forward, 2 homing), additional shooters.
local EXPECT = {
    [0] = { 0, 0, 0 }, { 0, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 },
    { 2, 1, 0 }, { 2, 1, 1 }, { 2, 1, 2 }, { 2, 2, 2 }, { 2, 2, 2 },
}

local NAMES = { [0] = "normal", "homing_dot", "missile", "laser", "shield", "spread_laser", "shooter_1",
                "shooter_2", "homing_missile", "shockwave" }

run_test(function()
    wait(60)
    press(KEY.R)            -- debug stage select: stage 2
    wait(5)
    press(KEY.START)
    wait(60)
    check("started on stage 2 at power 0", tel("stage") == 1 and tel("power") == 0, power_line())
    set_ctl("ctl_invincible", 1)

    for step = 0, 8 do
        set_ctl("ctl_set_power", step + 1)
        wait(3)
        local e = EXPECT[step]
        local ok = tel("power") == step and tel("gun") == e[1] and tel("missiles") == e[2] and tel("shooters") == e[3]
        check(string.format("step %d %s loadout", step, NAMES[step]), ok, power_line())

        -- Fire for a while; every step must put shots on screen.
        key_down(KEY.A)
        local most = 0
        for i = 1, 40 do
            wait(1)
            most = math.max(most, tel("player_shots"))
        end
        key_up(KEY.A)
        check(string.format("step %d fires", step), most > 0, "max shots " .. most)
        if step == 5 or step == 7 then
            key_down(KEY.A); wait(12); shot(string.format("step%d_%s", step, NAMES[step])); key_up(KEY.A)
        end
        if step == 7 then
            check("spread laser from ship + 2 shooters fills the screen", most >= 9, most)
        end
        wait(30)
    end

    check("shield granted at the SHIELD step", tel("shield") == 3, power_line())

    -- Shockwave: the first one fires as soon as the step is reached, then every 600 frames.
    local waves0 = tel("shockwaves")
    set_ctl("ctl_set_power", 10)
    local fired = wait_until(function() return tel("shockwaves") > waves0 end, 10)
    check("reaching SHOCKWAVE fires one immediately", fired, power_line())
    wait(1)
    shot("shockwave_flash")
    check("shockwave clears enemies and bullets", tel("enemies") == 0 and tel("enemy_bullets") == 0,
        snapshot_line())
    local again = wait_until(function() return tel("shockwaves") > waves0 + 1 end, 620, 1)
    local cpus = {}
    for i = 1, 14 do wait(1); cpus[#cpus + 1] = tel("cpu_pct") end
    log("cpu % in the frames after a shockwave: " .. table.concat(cpus, " "))
    check("shockwave repeats periodically (600 frames)", again, power_line())

    -- Losing a ship resets the ladder. Drop to the laser (no shield) and fly into the cave ceiling.
    set_ctl("ctl_set_power", 4)
    wait(3)
    check("power can go back down (test hook)", tel("power") == 3 and tel("shooters") == 0, power_line())
    set_ctl("ctl_invincible", 0)
    wait(40)                -- let any shockwave invulnerability run out
    key_down(KEY.UP)
    local died = wait_until(function() return tel("player_alive") == 0 end, 400)
    key_up(KEY.UP)
    check("ship destroyed", died, snapshot_line())
    check("death resets power to the normal shot", tel("power") == 0 and tel("gun") == 0 and tel("missiles") == 0
        and tel("shooters") == 0 and tel("shield") == 0, power_line())
    log("end: " .. snapshot_line() .. " " .. power_line())
end)
