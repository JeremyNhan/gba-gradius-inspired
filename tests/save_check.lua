-- Persistence check: run after full_run on the same ROM. full_run ends with a hi-score far above
-- the default (20000); after an emulator restart the title screen must load it back from SRAM.
-- The runner passes EXPECTED_HISCORE (read from full_run's result) via the environment wrapper.

run_test(function()
    wait(90)
    check("boots to TITLE", tel("state") == STATE.TITLE)
    local hi = tel("hiscore")
    log("hiscore after restart: " .. hi)
    check("hi-score loaded from SRAM after restart", hi > 20000, hi)
    if EXPECTED_HISCORE then
        check("hi-score matches previous run", hi == EXPECTED_HISCORE, hi .. " vs " .. EXPECTED_HISCORE)
    end
    shot("title_with_saved_hiscore")
end)
