-- Death / respawn / lives / game over on the DEBUG ROM (no invincibility).
-- Uses the debug stage select (R on the title screen) to start in stage 2, then flies into the
-- cave ceiling repeatedly, which is a deterministic way to lose every ship.

run_test(function()
    wait(60)
    press(KEY.R)            -- debug stage select: stage 2
    wait(5)
    press(KEY.START)
    wait(10)
    check("started on stage 2", tel("state") == STATE.PLAYING and tel("stage") == 1, snapshot_line())

    -- Wait (invincible, so stray enemies can't interfere) for the ceiling to grow.
    set_ctl("ctl_invincible", 1)
    wait(900)
    set_ctl("ctl_invincible", 0)
    wait(2)
    local lives0 = tel("lives")
    check("3 lives at start", lives0 == 3, lives0)

    key_down(KEY.UP)
    local died = wait_until(function() return tel("player_alive") == 0 end, 300)
    check("touching terrain destroys the ship", died, snapshot_line())
    check("a life is lost", tel("lives") == lives0 - 1, tel("lives"))
    wait(20)
    shot("01_explosion")
    key_up(KEY.UP)

    local respawned = wait_until(function() return tel("player_alive") == 1 end, 200)
    check("ship respawns", respawned, snapshot_line())
    wait(10)
    shot("02_respawn")

    -- Invulnerable right after respawn: flying into the ceiling must not kill immediately.
    local lives1 = tel("lives")
    key_down(KEY.UP)
    wait(60)
    check("respawn invulnerability", tel("player_alive") == 1 and tel("lives") == lives1, snapshot_line())

    -- Lose the remaining ships.
    local over = wait_until(function() return tel("state") == STATE.GAME_OVER end, 1500, 5)
    key_up(KEY.UP)
    check("losing all lives -> GAME_OVER", over, snapshot_line())
    check("deaths counted", tel("deaths") == 3, tel("deaths"))
    wait(100)
    shot("03_game_over")

    press(KEY.START)
    wait(30)
    check("START on game over -> TITLE", tel("state") == STATE.TITLE, STATE_NAME[tel("state")])
    shot("04_title")

    -- Restart works.
    press(KEY.START)
    wait(30)
    check("new game after game over", tel("state") == STATE.PLAYING and tel("lives") == 3 and tel("score") == 0,
        snapshot_line())
end)
