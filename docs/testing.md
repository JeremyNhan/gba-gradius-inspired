# Testing

## 1. How to run

```powershell
.\tests\run_tests.ps1            # build the test ROM, run every scenario in mGBA (two boots)
.\tests\run_tests.ps1 -NoBuild   # run the existing spaceshooter_test.gba
```

Requirements: the mGBA **nightly** Qt build (it has `--script`) in `tools\emulator\mGBA.exe` or `$env:MGBA`, and devkitARM's `arm-none-eabi-nm` (finds the result block in the ELF). A small emulator window opens during each boot. The emulator runs uncapped, so the whole suite takes about 20 seconds.

Results go to `tests/out/results_boot1.txt` and `results_boot2.txt`, screenshots to `tests/out/*.png` (git-ignored).

## 2. How the tests work

All scenarios and checks are **C code** compiled into a test ROM (`make TESTS=1`): the release game code plus `src/test/`.

* `test_frame()` runs every frame before the game logic. The active scenario is a protothread: `T_WAIT(t, frames)` and `T_WAIT_UNTIL(t, condition, max_frames)` return to the game loop and resume on a later frame, so scenarios read like sequential scripts.
* Scenarios drive the real game through `input_inject()` (the keypad is replaced by a key mask). They read game state directly (`game`, `world`, `app_state()`, pool counts). Where a feature needs it they use the test controls `test_ctl` (invincibility, autofire, set power level).
* Checks and log lines go to a text buffer in RAM (`ss_test_io`). Screenshot requests are a name plus a sequence counter.
* `tests/launcher.lua` (about 30 lines) is the only Lua: it takes the requested screenshots, and when the ROM sets `done` it writes the log to a file and exits mGBA.
* `run_tests.ps1` deletes the save, boots the ROM twice and fails unless both logs end with `RESULT: PASS`. The first boot runs smoke, death, power and full_run, then leaves a marker in SRAM. The second boot sees the marker and runs the save check.

| Suite | What it does |
|---|---|
| `smoke` | Boot → title → start → shooting → charged shot → pause/resume → movement → screen bounds → combat and scoring; audio state and hardware registers |
| `death` | Stage 2, flies into the cave ceiling until every life is lost → respawn, invulnerability, game over, back to title, new game |
| `power` | Every power-ladder step via `test_ctl.set_power`: loadout and firing; shield; shockwave fires at once, clears enemies and bullets, repeats every 600 frames; death resets to the normal shot |
| `full_run` | A bot (invincible + autofire, follows the boss vertically) plays all three stages, bosses, stage clears and the ending; records CPU, sprites, pools, per-system cost |
| `save_check` | Second boot: the high score from the first boot came back from SRAM |

## 3. Checklist (last run: 2026-09-24, C version: 91 checks, 0 failed)

| Item | Result | Evidence |
|---|---|---|
| ROM builds | PASS | release, debug and test builds with no warnings (`-Wall -Wextra`) |
| ROM boots, title screen | PASS | smoke: TITLE after 120 frames, title music playing |
| START, 8-way movement, screen bounds | PASS | smoke |
| Shooting, charged shot | PASS | smoke |
| Enemies spawn, collisions, scoring | PASS | smoke, full_run |
| Power capsules drop and advance the ladder | PASS | full_run: step 4 by the end of stage 1, 8 by stage 2, 9 in stage 3 |
| Every power step, shield, shockwave | PASS | power |
| Death, respawn, invulnerability, lives, game over, restart | PASS | death |
| Bosses: appear, take damage, die | PASS | full_run (all three) |
| Stage clear → next stage → ending → title | PASS | full_run |
| Pause (clock frozen, input ignored, music paused) | PASS | smoke |
| High score saved to SRAM and reloaded after restart | PASS | save_check |
| Audio hardware running | PASS | smoke: master enable, Direct Sound A/B, timer 0 (Maxmod) |
| No missed gameplay frames | PASS | full_run: 0 over 18,001 gameplay frames (the one-frame stage loads behind a black fade are excluded) |
| Sprite budget | PASS | peak 68 of 128, none dropped |
| No visible corruption | PASS (visual) | screenshots of every stage, boss, clear, game over and ending pages inspected |

## 4. Performance (test ROM = release code, full playthrough)

| Metric | Measured | Limit |
|---|---|---|
| Frame rate | 59.73 Hz, 0 missed gameplay frames | hardware refresh |
| CPU, average gameplay frame | 28.8 % | 100 % |
| CPU, worst frame | 74 % | 100 % |
| Hardware sprites, peak | 68 | 128 |
| Player shots / enemies / enemy bullets, peak | 32 / 10 / 32 | pools 32 / 16 / 32 |
| ROM size | 253 KB (release) | 32 MB |

Per-system cost from `prof_mark()` (1/1000 of a frame, average / peak): shots 44/141, collide 38/105, render 54/146, enemies 6/38, bullets 5/52, boss 1/34, hud 2/90, everything else ≤ 5 on average. The rest of the frame is the VBlank commit and Maxmod mixing.

The earlier Butano/C++ version measured 31.4 % average and 85 % worst frame on the same playthrough. The C version is cheaper mainly because sprites are rebuilt into a shadow OAM each frame, so there is no sprite creation or sorting cost.

## 5. Real hardware assessment

**Not tested on a physical GBA.** Points that make it plausible:

* Timing: logic runs once per VBlank; the worst frame uses 74 % of the budget.
* Wait states are set explicitly (`WS_STANDARD`, as commercial games do). The emulator-only fast path of the power-on default is not relied on.
* SRAM is accessed with byte reads and writes only (8-bit bus); the ROM carries the `SRAM_V113` tag. A flash cart must use the SRAM save type.
* Audio: Maxmod (Direct Sound + DMA + timer 0), the standard approach.
* No emulator-only features: the test ROM only exposes a RAM block; the release ROM has no test code.

Remaining risks: behaviour on real sprite-per-scanline limits in dense scenes (only emulated by mGBA), and flash-cart save detection.

## 6. Manual test pass

1. `.\build.ps1 -Run`
2. Title: logo, star scroll, music → START
3. Stage 1: A to shoot, hold/release B for the charged wave, collect P capsules (label names the new power; the bottom line shows the loadout)
4. START pauses and resumes
5. Get hit: explosion, respawn blink, life counter drops, power resets
6. Boss: WARNING banner + siren, boss bar under the score
7. Reach the shockwave (step 9): white flash and a cleared screen every 10 s
8. Debug ROM (`.\build.ps1 -DebugBuild`): L/R on the title picks the stage, SELECT toggles the CPU/entity overlay, L+R skips to the boss
