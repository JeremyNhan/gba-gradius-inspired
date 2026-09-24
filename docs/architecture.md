# Architecture

Space Shooter is written in C (gnu11) on **libtonc** (hardware definitions, BIOS calls, sine table) and **Maxmod** (music and sound mixing), built with devkitARM. There is no engine layer: the game owns its main loop, its VRAM layout and its sprite list. Nothing is allocated at run time; all state is static.

## 1. Build pipeline

```
tools/assetgen/*.c   (host C: pixel-art drawing code, palettes, font, song scores, SFX synthesis)
      │  host gcc -> tools/assetgen/bin/assetgen   (built by the Makefile)
      ▼
assets/generated/gen_gfx.c + gen_gfx.h   GBA-ready data: 4bpp tiles, BG maps, BGR555 palettes, font rows
assets/generated/audio/*.mod, *.wav      music (ProTracker MOD) and effects (8-bit WAV)
      │  mmutil -> soundbank.bin (+ bin2o)
      ▼
src/**/*.c + gen_gfx.c  --arm-none-eabi-gcc (Thumb, -O2)-->  spaceshooter.elf  --objcopy + gbafix-->  spaceshooter.gba
```

* The Makefile runs the generator first, then compiles in a second make pass so the file wildcards see the generated sources.
* The generator rewrites a file only when its bytes change, so incremental builds stay incremental.
* The art and audio definitions are C code. They produce exactly the same pixels and samples as the original Python generator: all 40 images and 23 audio files were compared byte for byte during the port.

Build variants:

| | `make` | `make DEBUG=1` | `make TESTS=1` |
|---|---|---|---|
| Output | `spaceshooter.gba` | `spaceshooter_debug.gba` | `spaceshooter_test.gba` |
| Debug overlay (SELECT), stage select (L/R), boss skip (L+R) | no | yes | no |
| Test controls (`test_ctl`: invincible, autofire, set power, skip to boss) | no | yes | yes |
| Test driver + scenarios (`src/test`), per-system profiler | no | no | yes |

## 2. Folder structure

```
Makefile, build.ps1, build.sh
tools/assetgen/     asset generator (host C): canvas, palettes, font, sprites, backgrounds, music, sfx, main
src/main.c          boot, interrupts, main loop
src/ss_app.c        top-level state machine
src/core/           hardware layer: video, sprites (shadow OAM), text, input, audio, save, rng, frame timer, base math
src/data/           constant tables: enemies, guns, power ladder, stage timelines and terrain keys
src/game/           gameplay: world, player, shots, enemies, bullets, boss, effects, powerups, hud, level (terrain,
                    backgrounds, stage runner)
src/screens/        title and ending
src/test/           test driver and scenarios (test ROM only)
tests/              mGBA launcher (Lua) and runner (PowerShell)
```

## 3. Main loop and timing

```c
while(1)
{
    VBlankIntrWait();          // BIOS halt until VBlank (IRQ handler: counter + mmVBlank)
    frame_timer_start();       // timers 2+3 cascaded, CPU cycles
    sprites_commit();          // shadow OAM -> OAM
    text_commit();             // changed text rows -> BG0 tiles + map
    terrain_commit();          // new terrain map columns
    video_commit();            // scroll registers, brightness fade, DISPCNT
    audio_frame();             // mmFrame (mixing), effect cooldowns
    input_update();
    sprites_begin();
    app_update();              // game logic + draw this frame's sprites
}
```

Everything that touches display memory happens right after VBlank begins; the logic then prepares the next frame. The game runs once per hardware frame (59.73 Hz) with no variable timestep, so it is deterministic.

`REG_WAITCNT` is set to `WS_STANDARD` (3/1 ROM wait states + prefetch) at boot. With the power-on default (4/2, no prefetch) every frame cost about 45 % more CPU time, because the Thumb code runs from ROM: the full playthrough averaged 42 % CPU before the change and 29 % after.

## 4. State machine

`game_state { TITLE, PLAYING, PAUSED, STAGE_CLEAR, GAME_OVER, ENDING }`. One handler per state returns the next state; `app_enter()` performs the transition (screen setup, music, overlays, fades).

| From | Event | To |
|---|---|---|
| TITLE | START | PLAYING (new session) |
| PLAYING | START | PAUSED |
| PAUSED | START | PLAYING |
| PLAYING | stage boss destroyed | STAGE_CLEAR |
| PLAYING | last life lost | GAME_OVER |
| STAGE_CLEAR | outro done + fade | PLAYING (next stage) or ENDING |
| GAME_OVER / ENDING | START / timer | TITLE |

PAUSED, STAGE_CLEAR and GAME_OVER keep drawing the frozen world (`world_render`) without updating it.

## 5. Memory

* No heap. Entity systems are fixed arrays with an `active` flag and deterministic iteration order. A full pool drops the spawn (counted), never overwrites.
* IWRAM (32 KB): globals, pools, shadow OAM, stack. EWRAM (256 KB): the 19 KB text tile buffer and the test log.
* ROM: code (Thumb), constant tables and all generated graphics (`const` arrays are never copied to RAM).

| Pool | Capacity |
|---|---|
| Player shots | 32 |
| Enemies | 16 |
| Enemy bullets | 32 |
| Effects | 16 |
| Power capsules | 4 |

## 6. Video

* **Mode 0**, four regular 256x256 backgrounds, 4bpp:

| Layer | Content | Priority | Scroll |
|---|---|---|---|
| BG0 | text / HUD (6 px font) | 0 | fixed |
| BG1 | terrain (streamed) or title logo | 1 | x1 |
| BG2 | stage backdrop | 2 | x0.5 |
| BG3 | starfield | 3 | x0.25 |

* **VRAM**: BG tiles in charblocks 0-1 (stars at 0, backdrop/logo at 128, terrain at 560), text tiles in charblocks 2-3 (20 rows x 30 tiles), maps in screenblocks 28-31. OBJ VRAM: every common sheet (507 tiles) loaded at boot, the current stage's boss sheets (up to 328 tiles) at tile 512.
* **Palettes**: BG banks 0 stars (colour 0 = backdrop), 1 backdrop/logo, 2 terrain, 3-6 font colours. OBJ banks 0 master, 1 boss, 2 all-white hit flash.
* **Sprites**: there are no persistent sprite objects. Each frame every system draws its sprites into a shadow OAM in front-to-back order (HUD, player, effects, bullets, shots, capsules, enemies, boss). The VBlank commit copies it with `oam_copy`. Drawing a sprite is a few dozen instructions and there is no creation cost. Butano's sprite creation had been the main CPU peak of the C++ version.
* **Text**: each text row owns 30 BG tiles. Strings are OR-ed into those tiles glyph row by glyph row (a 6 px pitch means shifting nibbles across tile edges). Changed rows are DMA-copied during VBlank. Colours are per 8 px cell (BG palette bank).
* **Terrain**: a 32x32 map on BG1 that wraps horizontally. When the scroll enters a new 8 px column, the column about to appear is rewritten in the next VBlank. Heights of the 32 columns in the map are cached for collision.
* **Fades and flashes**: the hardware brightness effect (`REG_BLDCNT`/`REG_BLDY`) fades to black or white on chosen layers. No palette maths is needed.
* **Camera shake**: an offset added to BG scroll registers and subtracted from gameplay sprite positions (the HUD ignores it).

## 7. Math and collision

* 20.12 fixed point (`fx`), `fx_mul` via a 64-bit multiply. No floating point at run time.
* Angles: 16-bit binary angles; `direction()` uses libtonc's `lu_sin/lu_cos` table; `angle_to()` uses the BIOS `ArcTan2`.
* Collision: centre + half-extent AABBs (`hitbox`), tested pairwise over the small pools each frame.
* Random: xorshift32 seeded per stage, so a play with the same inputs is always identical.

## 8. Input

`input_update()` reads `REG_KEYINPUT` once per frame. Test builds can inject a key mask instead (`input_inject`), so the test scenarios drive the real game code.

| Key | Action |
|---|---|
| D-pad | 8-way movement |
| A (hold) | main gun + homing dots + missiles (auto-fire) |
| B (hold, release) | charged wave |
| START | pause / resume; start from title |
| SELECT, L, R | debug ROM only |

## 9. Audio

Maxmod with 8 mixing channels (4 for the MOD songs, 4 for effects). `mmVBlank` runs in the VBlank interrupt, `mmFrame` once per frame. Rapid effects have a per-effect cooldown so shots cannot flood the effect channels. Jingles play once (`MM_PLAY_ONCE`); stage and boss songs loop.

## 10. Save

32 KB SRAM at 0x0E000000, byte accesses only: `"SSHS"`, version, high score and checksum. The ROM contains the `SRAM_V113` string so emulators and flash carts detect the save type. The high score is written when returning to the title or on game over.

## 11. Testing

See [testing.md](testing.md). The test ROM runs scenarios written in C (`src/test`) and reports through a RAM block that a 30-line Lua launcher saves.
