-- Smoke test on the RELEASE ROM (no debug hooks): boot, title, start, shooting, pause,
-- movement, screen bounds, enemies, scoring, audio/video timing.

run_test(function()
    wait(5)
    check("telemetry block found", magic_ok())

    wait(120)
    check("boots to TITLE", tel("state") == STATE.TITLE, STATE_NAME[tel("state")])
    shot("01_title")

    local snd = sound_status()
    check("title music playing", tel("music_playing") == 1)
    check("sound hardware enabled (master, Direct Sound, sample timer)", snd.master and snd.dsound and snd.timer,
        snd.raw)

    press(KEY.START)
    wait(20)
    check("START begins game", tel("state") == STATE.PLAYING, STATE_NAME[tel("state")])
    check("starts on stage 1", tel("stage") == 0)
    check("starts with 3 lives", tel("lives") == 3, tel("lives"))
    wait(40)
    shot("02_stage_banner")

    -- Shooting
    key_down(KEY.A)
    wait(15)
    check("A fires shots", tel("player_shots") > 0, tel("player_shots"))
    shot("03_shooting")
    key_up(KEY.A)

    -- Charged shot: hold B past the charge time, release, one piercing wave comes out.
    wait(45)
    check("normal shots leave the screen", tel("player_shots") == 0, tel("player_shots"))
    hold(KEY.B, 60)
    wait(2)
    check("B charge + release fires a charged shot", tel("player_shots") > 0, tel("player_shots"))

    -- Pause freezes gameplay completely
    press(KEY.START)
    wait(5)
    check("START pauses", tel("state") == STATE.PAUSED, STATE_NAME[tel("state")])
    local sf = tel("stage_frame")
    local px = tel("player_x")
    key_down(KEY.RIGHT)
    wait(90)
    key_up(KEY.RIGHT)
    check("pause freezes stage clock", tel("stage_frame") == sf, sf .. " vs " .. tel("stage_frame"))
    check("pause ignores movement input", tel("player_x") == px, px .. " vs " .. tel("player_x"))
    check("music paused while paused", tel("music_playing") == 0)
    shot("04_paused")
    press(KEY.START)
    wait(10)
    check("START resumes", tel("state") == STATE.PLAYING, STATE_NAME[tel("state")])
    check("stage clock runs again", tel("stage_frame") > sf)
    check("stage music resumes", tel("music_playing") == 1)

    -- Movement
    local x0, y0 = tel("player_x"), tel("player_y")
    hold(KEY.RIGHT, 30)
    check("RIGHT moves ship", tel("player_x") > x0, x0 .. " -> " .. tel("player_x"))
    hold(KEY.DOWN, 20)
    check("DOWN moves ship", tel("player_y") > y0, y0 .. " -> " .. tel("player_y"))

    -- Screen bounds (only meaningful while the ship is alive)
    hold(KEY.UP, 150)
    check("ship stays below HUD", tel("player_y") >= -64, tel("player_y"))
    hold(KEY.LEFT, 200)
    check("ship stays inside left edge", tel("player_x") >= -112, tel("player_x"))
    hold(KEY.DOWN, 200)
    check("ship stays above status line", tel("player_y") <= 64, tel("player_y"))
    hold(KEY.RIGHT, 250)
    check("ship stays inside right edge", tel("player_x") <= 112, tel("player_x"))

    -- Combat: sweep up and down while firing; enemies should spawn and be destroyed.
    hold(KEY.LEFT, 150)
    local max_enemies = 0
    on_frame(function()
        local e = tel("enemies")
        if e > max_enemies then max_enemies = e end
    end)
    key_down(KEY.A)
    local dir = KEY.UP
    for i = 1, 14 do
        hold(dir, 40)
        dir = (dir == KEY.UP) and KEY.DOWN or KEY.UP
        wait(20)
        if i == 7 then shot("05_combat") end
    end
    key_up(KEY.A)
    check("enemies spawned", max_enemies > 0, "max on screen " .. max_enemies)
    check("score increased (enemies destroyed)", tel("score") > 0, tel("score"))
    check("no gameplay frame missed", tel("missed_frames") == 0, tel("missed_frames"))
    log("deaths during smoke run: " .. tel("deaths"))
    log("end: " .. snapshot_line())
end)
