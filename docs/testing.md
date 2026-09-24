# Testing

## 1. How to run

```powershell
.\tests\run_all.ps1            # build release + debug + profile ROMs, then run every suite
.\tests\run_all.ps1 -NoBuild   # run the suites against the ROMs already built
.\tests\run_tests.ps1 -Test full_run -Build profile -Fast   # one suite
```

Requirements: the mGBA **nightly** Qt build (it has `--script`) in `tools\emulator\mGBA.exe` or `$env:MGBA`, and devkitARM's `arm-none-eabi-nm` (used to find the telemetry block in the ELF). A small emulator window opens during each run. `-Fast` disables video/audio sync, so a full playthrough takes about 9 seconds.

Results go to `tests/out/<test>_result.txt` and screenshots to `tests/out/<test>_*.png` (git-ignored).

## 2. How the tests work

The game mirrors its state into `ss_telemetry` (`src/core/ss_telemetry.h`), a fixed struct in RAM with state, stage, score, lives, entity counts, CPU usage, VRAM usage, and more. `run_tests.ps1` reads its address from the ELF symbol table and passes it to the Lua script. The harness (`tests/lib/harness.lua`) then:

* injects key presses with `emu:setKeys`
* reads telemetry with `emu:read8/16/32`
* reads the GBA sound registers directly (SOUNDCNT, timer 0) to prove audio hardware is running
* takes screenshots with `emu:screenshot`
* writes `[PASS]/[FAIL]` lines and a summary, then exits mGBA

Debug and profile builds also accept test controls written by the script: `ctl_invincible`, `ctl_autofire`, `ctl_skip_to_boss`. The release ROM ignores them (`SS_TEST_HOOKS=0`).

| Suite | ROM | What it does |
|---|---|---|
| `smoke` | release | Boot → title → start → shoot → charge shot → pause/resume → movement → screen bounds → combat and scoring |
| `death` | debug | Starts in stage 2, flies into the cave ceiling until all lives are lost → respawn, invulnerability, game over, back to title, new game |
| `power` | debug | Starts in stage 2; uses the `ctl_set_power` test hook to visit every step of the power ladder, checks each step's loadout and that it fires; shield granted; shockwave fires on reaching step 9, clears enemies and bullets, repeats after 600 frames; losing a ship resets to the normal shot |
| `full_run` | debug and profile | Bot plays all 3 stages (invincible + autofire, dodging vertically); skips to each boss after the stage's enemy waves, kills it, goes through stage clear → ending → title; collects performance statistics |
| `save_check` | profile | Run after `full_run`: restarts the emulator and checks that the high score came back from SRAM |

## 3. Checklist (last run: 2026-09-24, all automated checks passing: 121 checks, 0 failed)

| Item | Result | Evidence |
|---|---|---|
| ROM builds | PASS | `build.ps1` release/debug/profile, no warnings |
| ROM output is valid | PASS | Header checked: title `SPACESHOOTER`, code `SSHT`, fixed byte 0x96, header complement checksum correct, `SRAM_V113` tag present |
| ROM boots | PASS | smoke: telemetry block found, state TITLE |
| Title screen works | PASS | smoke `01_title.png`, title music playing |
| START works | PASS | smoke: TITLE → PLAYING |
| Player moves (8-way) | PASS | smoke: RIGHT/DOWN change position |
| Player cannot leave screen | PASS | smoke: held each direction for 150–250 frames, position clamped |
| Player shoots | PASS | smoke: A → shots |
| Charged shot | PASS | smoke: hold B 60 frames, release → shot |
| Enemies spawn | PASS | smoke, full_run |
| Enemy projectiles work | PASS | full_run: up to 32 enemy bullets on screen |
| Collision works | PASS | smoke: score rises (shots hit enemies); death: terrain kills the ship; full_run: bosses take damage |
| Power capsules drop randomly from kills | PASS | full_run: capsules drop; the bot climbs to step 4 in stage 1, step 8 in stage 2, step 9 (shockwave) in stage 3 |
| Every power step works | PASS | power: loadout and firing checked for steps 0–8; up to 31 player shots with spread laser + 2 shooters |
| Shield | PASS | power: 3-hit shield granted at step 4 |
| Shockwave | PASS | power: fires on reaching step 9, clears all enemies and bullets, repeats after 600 frames |
| Death resets power | PASS | power: laser + missiles → ship destroyed → normal shot, no shooters, no shield |
| Player death works | PASS | death: explosion screenshot, `player_alive` = 0 |
| Respawn works | PASS | death: respawn + invulnerability window |
| Lives decrement | PASS | death: 3 → 2 → … → 0 |
| Game over works | PASS | death: GAME_OVER, START → TITLE, new game starts cleanly |
| Boss works / boss HP | PASS | full_run: each of the 3 bosses appears, has HP, takes damage, dies |
| Stage clear works | PASS | full_run: stage 1 → 2 → 3 |
| Game can be completed | PASS | full_run: final boss → ENDING → TITLE, high score updated |
| Pause works | PASS | smoke: stage clock frozen, input ignored, music paused, resumes |
| Save (high score) | PASS | save_check: high score survives an emulator restart |
| Audio works | PASS (hardware state) | smoke: Maxmod running, Direct Sound + timer 0 enabled, music playing/pausing. Sound quality was not judged by these automated runs; listen in mGBA to judge the mix |
| No visible corruption | PASS (visual) | Screenshots of every stage, boss, clear screen and ending page inspected |
| No major frame drops | PASS | full_run: `missed_frames` = 0 over ~19,100 gameplay frames |

## 4. Performance measurements (profile ROM = release code, full playthrough)

| Metric | Measured | Limit |
|---|---|---|
| Frame rate | 59.73 Hz, 0 missed frames | hardware refresh |
| CPU, average gameplay frame | 31.4 % | 100 % |
| CPU, worst frame | 85 % (stage 3 boss: 32 bullets, full-power volley and a capsule pickup in one frame) | 100 % |
| Hardware sprites, peak | 85 | 128 (OAM) |
| OBJ VRAM tiles, peak | 241 4bpp tiles (7.5 KB) | 1024 (32 KB) |
| BG VRAM tiles, peak | 704 | 2048 per 64 KB BG VRAM |
| Enemy bullets, peak | 32 | pool of 32 |
| Enemies, peak | 9 | pool of 16 |
| Player shots, peak | 32 | pool of 32 |
| ROM size | 335,772 bytes (328 KB) | 32 MB |

When a pool is full, spawn requests are dropped (`pool_drops` in telemetry, about 40 per full run, almost all of them during boss fights). This is the intended graceful degradation: nothing is overwritten and the frame budget holds.

The debug build is slower (asserts on). Its worst frame measured 90 %, still with no missed frames.

### Per-system profile

In test-hook builds `world::update()` times each system with `bn::timer` and writes the cost to `ss_telemetry.prof[]`; `full_run` logs the average and peak per system. Last profile run (% of a frame, average / peak):

| stage | player | shots | enemies | boss | bullets | powerups | collide | effects | terrain | bgs | hud |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 0.0 / 16.9 | 2.1 / 11.4 | 6.6 / 19.5 | 0.7 / 6.3 | 0.1 / 13.8 | 0.9 / 13.7 | 0.3 / 1.9 | 3.7 / 24.4 | 0.8 / 6.7 | 0.1 / 1.9 | 0.1 / 0.4 | 0.4 / 16.6 |

`prof_app` also times the whole `app::update()`. Note that `cpu_pct` (`bn::core::last_cpu_usage()`) describes the *previous* frame, so `full_run` pairs each heavy frame with the previous frame's profile.

Game logic averages about 16 % of a frame; the rest of the ~29 % is Butano's frame commit (OAM, BG maps, palettes) and Maxmod mixing. Peaks are single frames that create many sprites (boss phase change, big explosions, HUD text redraw).

The profile found one real hotspot. Terrain collision interpolated the terrain key list (a linear search plus two software divisions, since the ARM7 has no divide instruction) for every shot and bullet each frame. The terrain now caches the ceiling/floor height of the 32 columns in its map ring. That cut the shot system from 8.1 % to 4.9 %, the average frame from 33.4 % to 29.4 %, and the worst frame from 88 % to about 78 %.

The power ladder (up to 32 player shots) brought new peaks. They all came from **sprite creation** (about 2 % of a frame each) and **text rendering** (up to 15 % for a full line), not from per-frame updates:

* `make_sprite` builds sprites with `bn::sprite_builder`, setting priority, z order and camera before insertion. Setting them afterwards re-sorted the sprite once per setter.
* Enemy bullets create their sprites in `update()`, at most 4 per frame. A mine's 8-bullet burst or a boss ring no longer creates 8+ sprites in one frame. Bullets collide from the frame they are fired.
* The additional shooters fire the ship's volley one frame after another (at most 3 lasers created per frame).
* The HUD redraws at most one text item per frame (pickup label, then status line, then score).
* A shockwave cancels bullets without sparks and delays its big explosion by a frame.

Together these took the boss system's peak from 30 % to 14 % and the worst frame from 91 % to 85 %.

## 5. Real hardware assessment

**Not tested on a physical GBA.** The following points make it plausible that it runs:

* Timing: all logic runs once per VBlank through `bn::core::update()`; the worst frame uses about 78 % of the budget, leaving margin for emulator/hardware differences.
* Memory: no heap during gameplay; large objects in EWRAM via `BN_DATA_EWRAM_BSS`; Butano's own IWRAM/stack use.
* Save: standard 32 KB SRAM accessed with 8-bit reads/writes through `bn::sram`; the ROM carries the `SRAM_V113` tag. A flash cart must be set to SRAM save type.
* Audio: Maxmod with Direct Sound + DMA, the standard GBA approach used by commercial games.
* No emulator-only features (no mGBA debug registers in the release ROM: Butano logging is disabled there).

Risks that remain on hardware: exact sprite-per-scanline limits in dense boss phases (tested only in mGBA, which emulates them), and flash-cart save-type detection.

## 6. Manual test pass

For a human check in mGBA (stable 0.10.5 or nightly):

1. `.\build.ps1 -Run`
2. Title: logo, star scroll, music → START
3. Stage 1: shoot with A, hold/release B for the charged wave, pick up P capsules (the label names the new power; the bottom line shows the loadout)
4. START to pause/resume
5. Let an enemy hit you: explosion, respawn blink, life counter drops
6. Reach the boss: WARNING banner + siren, boss bar at the top
7. Reach the shockwave (step 9): the screen flashes white and clears every 10 s
8. Debug ROM (`.\build.ps1 -DebugBuild`): L/R on title picks the stage, SELECT toggles the CPU/entity overlay, L+R in-game skips to the boss
