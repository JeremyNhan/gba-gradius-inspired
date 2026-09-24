# Architecture (Phase 2)

## 1. Stack

```
tools/*.py  (pixel art as ASCII grids/drawing code, palettes, song scores; plain text)
      │  tools/gen_assets.py   (Python 3, stdlib only; deterministic)
      ▼
assets/generated/graphics/*.bmp + *.json   assets/generated/audio/*.mod, *.wav
      │  Butano build (grit → tiles/palettes, mmutil → soundbank)
      ▼
C++20 game code (src/*) + Butano 21.8.0 (external/butano)
      │  devkitARM r68 (arm-none-eabi-g++ 16.1, Thumb by default)
      ▼
spaceshooter.elf → objcopy → gbafix → spaceshooter.gba
```

The Makefile's `EXTTOOL` hook runs the asset generator before each build. The generator only rewrites a file when its bytes change, so incremental builds stay incremental.

## 2. Folder structure

```
Makefile              Butano project makefile (TARGET=spaceshooter)
build.ps1 / build.sh  one-command build (handles path-with-space via subst)
external/butano       Butano engine, git submodule pinned to tag 21.8.0
tools/                asset sources + generator (sprites, backgrounds, font, palettes, music, SFX)
assets/generated/     build output of tools/ (git-ignored)
src/
  main.cpp            boot + main loop
  ss_app.cpp/.h       top-level state machine
  core/               app-wide helpers: constants, pool, collision, save, telemetry/debug, text
  game/               gameplay: world, player, weapons, shots, enemies, bullets, powerups, boss, terrain, stage runner, effects, hud, backgrounds
  data/               constexpr tables: weapons, enemies, stages, bosses
  screens/            title and ending screens (pause, stage clear and game over are overlays drawn by ss_app)
tests/                mGBA Lua test scripts + runner
docs/                 research, architecture, design, testing
```

## 3. Game loop & state machine

`main()` calls `bn::core::init()`, then loops `app.update(); bn::core::update();`. `bn::core::update()` blocks until VBlank and commits sprite, BG and palette changes during VBlank. Logic runs once per 59.73 Hz frame, with no variable timestep, so gameplay is deterministic.

```
enum class game_state { TITLE, PLAYING, PAUSED, STAGE_CLEAR, GAME_OVER, ENDING };
```

| From | Event | To |
|---|---|---|
| TITLE | START | PLAYING (new session, stage 1) |
| PLAYING | START | PAUSED |
| PAUSED | START | PLAYING |
| PLAYING | boss destroyed (stage 1, 2) | STAGE_CLEAR |
| PLAYING | final boss destroyed | ENDING |
| PLAYING | last life lost | GAME_OVER |
| STAGE_CLEAR | timer | PLAYING (next stage, new world) |
| GAME_OVER / ENDING | START / timer | TITLE |

A single `switch` in `app::update()` dispatches to one handler per state. Each handler returns the next state, and `app::_enter(state)` performs transitions: it creates and destroys screen objects and changes music. PAUSED keeps the `world` object alive but does not update it.

## 4. Memory strategy

* **No heap.** All scenes live in one EWRAM static block (`BN_DATA_EWRAM_BSS`) as `bn::optional<…>` members, constructed and destroyed at transitions.
* Entities live in **fixed-size pools** (`core/ss_pool.h`: array + active flags, deterministic iteration order). When a pool is full, the spawn request is **dropped** (returns `nullptr`) and a counter in telemetry records it. It never overwrites memory or crashes.
* IWRAM holds only the stack, Butano internals, the telemetry block, and small globals.
* Read-only data (enemy, weapon, stage and boss tables) is `constexpr` and stays in ROM.

## 5. Entity model

This is not a generic ECS. Each entity kind is a plain struct with `bn::fixed_point` position and velocity, an index into a `constexpr` definition table, and a `bn::optional<bn::sprite_ptr>`. Systems are free functions or small classes over pools, updated in a fixed order:

```
stage_runner (spawn events) → player (input, move, fire) → player_shots → enemies (AI, fire)
→ boss → enemy_bullets → powerups → collisions → effects → hud → camera shake
```

## 6. Rendering model

* **Mode 0**, four regular backgrounds:
  * BG3 far starfield (256×256, scroll ×0.25)
  * BG2 stage backdrop (nebula / cave wall / hull, scroll ×0.5)
  * BG1 terrain, streamed dynamically: a 32×32-cell map whose columns are rewritten from stage height data as the camera advances (scroll ×1)
  * BG0 title logo on the title screen only
* **Sprites**: 4bpp, 1D mapping. One shared 16-color master palette for most gameplay sprites (consistent look, one palette bank), a separate boss palette, a font palette, and a white "flash" palette used for hit feedback.
* **Sprite budget (OAM = 128):**

| Group | Cap |
|---|---|
| Player + shield + charge glow | 3 |
| Player shots | 20 |
| Enemies | 16 |
| Enemy bullets | 36 |
| Effects (explosions/sparks) | 16 |
| Powerups | 4 |
| Boss parts | 6 |
| HUD + boss bar + text | ≤ 20 |
| **Total** | **≤ 121** |

* Camera: a `bn::camera_ptr` is attached to gameplay sprites and BGs for screen shake; the HUD is not attached.

## 7. Input

Butano `bn::keypad` (read once per frame by `bn::core`). Mapping:

| Key | Action |
|---|---|
| D-pad | 8-way movement |
| A (hold) | primary weapon (auto-fire) + missiles |
| B (hold, release) | charge shot |
| START | pause / resume; start from title |
| SELECT | debug overlay (debug build only) |

## 8. Audio

Maxmod through `bn::music` / `bn::sound`. Music is generated 4-channel ProTracker MODs: title, 3 stages, boss, ending, plus stage-clear and game-over jingles. SFX are 8-bit mono WAV. Rapid SFX (shots) are rate-limited to protect Maxmod's 4 SFX channels.

## 9. Save

`bn::sram` stores `{ magic "SSHS", version, high score, checksum }`. Invalid data resets to a default high score. The high score is written only on GAME_OVER or ENDING when beaten.

## 10. Build pipeline

`build.ps1` (Windows):
1. Locate devkitPro (`C:\devkitPro`) and Python (`py -3`).
2. `subst` a free drive letter to the repo (paths with spaces break make).
3. Run MSYS2 bash: `make -j$(nproc) PYTHON=…` (release), `DEBUG=1` (`-DebugBuild`) or `PROFILE=1` (`-ProfileBuild`).
4. Remove the drive mapping.

`build.sh` (Linux/MSYS2 shell): `make -j$(nproc)`.

Build variants:

| | Release (`make`) | Debug (`make DEBUG=1`) | Profile (`make PROFILE=1`) |
|---|---|---|---|
| Output | `spaceshooter.gba` | `spaceshooter_debug.gba` | `spaceshooter_profile.gba` |
| Butano asserts / logging | off | on | off |
| Debug overlay, stage select, L+R skip | compiled out (`SS_DEBUG=0`) | on | compiled out |
| Telemetry block | on (read-only state for tests) | on + test controls | on + test controls (`SS_TEST_HOOKS=1`) |

The profile build runs release-optimised code with the test controls, so CPU measurements reflect the shipped ROM.

## 11. Testing

`tests/run_tests.ps1` launches the mGBA nightly (Qt, `--script`) with Lua scripts that inject input, read the telemetry block (address from `arm-none-eabi-nm` on the ELF), take screenshots, and write a result file. See docs/testing.md.
