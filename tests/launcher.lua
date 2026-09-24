-- mGBA launcher for the C test ROM (spaceshooter_test.gba). All scenarios and checks are C code in
-- src/test/; this script only takes the screenshots the ROM asks for and saves its result log.
-- run_tests.ps1 defines TEST_IO (address of ss_test_io), OUT_DIR and RESULT_FILE before loading it.
--
-- ss_test_io layout: +8 done, +12 shot_seq, +16 shot_name[32], +48 log_len, +52 ready, +56 log[]

local last_seq = 0

local function read_string(address, max_len)
    local chars = {}
    for i = 0, max_len - 1 do
        local c = emu:read8(address + i)
        if c == 0 then break end
        chars[#chars + 1] = string.char(c)
    end
    return table.concat(chars)
end

callbacks:add("frame", function()
    emu:write32(TEST_IO + 52, 1)        -- tell the ROM the launcher is listening

    local seq = emu:read32(TEST_IO + 12)
    if seq ~= last_seq then
        last_seq = seq
        emu:screenshot(OUT_DIR .. "/" .. read_string(TEST_IO + 16, 32) .. ".png")
    end

    if emu:read32(TEST_IO + 8) == 1 then
        local f = io.open(RESULT_FILE, "w")
        f:write(read_string(TEST_IO + 56, emu:read32(TEST_IO + 48)))
        f:close()
        os.exit(0)
    end
end)
