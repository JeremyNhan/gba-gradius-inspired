# Research Report — GBA Development (Phase 1)

> **Update (C rewrite, v0.0.1-alpha):** the game no longer uses Butano, Python or Lua test logic. Section 0 records the decisions for the C version; sections 1-8 are the original Phase 1 research, kept for reference (the hardware facts in section 3 still apply).

## 0. C rewrite (2026-09-24)

| Topic | Decision |
|---|---|
| Language / libraries | C (gnu11) on **libtonc** (register and memory definitions, BIOS calls, sine table) + **Maxmod** (music/effects mixing), both installed by devkitPro's `gba-dev` group **(verified: `/c/devkitPro/libtonc`, `libgba/lib/libmm.a`)** |
| Build | devkitPro's `gba_rules` Makefile template (as in `/c/devkitPro/examples/gba/template`), extended with a first pass that builds and runs the host asset generator |
| Asset pipeline | `tools/assetgen`, host C built with MSYS2 `gcc` (`pacman -S gcc`). It writes GBA-ready C arrays directly (no grit), plus MOD/WAV for `mmutil`. **(verified)** Its output is byte-identical to the former Python generator for all 40 images and 23 audio files |
| Rounding | The art uses Python's `round()` (round half to even); C uses `nearbyint()` in the default rounding mode, and Python's `%` on negative numbers is reproduced with a helper |
| Sprites | Shadow OAM rebuilt every frame and copied in VBlank with `oam_copy` ([tonc: sprites](https://gbadev.net/tonc/regobj.html)) |
| Text | BG0 text with a 6 px pitch rendered into per-row tiles (a fixed 8x8 tile font would not fit the 33-character HUD line) |
| Fades | Hardware brightness (`REG_BLDCNT` + `REG_BLDY`, [tonc: blending](https://gbadev.net/tonc/gfx.html)) instead of palette recomputation |
| Wait states | `REG_WAITCNT = WS_STANDARD` (0x4317: ROM 3/1 + prefetch, SRAM 8 cycles; [GBATEK: waitstate control](https://mgba-emu.github.io/gbatek/)). **(verified)** Without it the full playthrough averaged 42 % CPU; with it, 29 % |
| Audio | `mmInitDefault(soundbank, 8)`, `mmVBlank` in the VBlank interrupt, `mmFrame` once per frame, as in devkitPro's `examples/gba/audio/maxmod/basic_sound` |
| Save | Byte-wise SRAM access (8-bit bus) + a `SRAM_V113` string in the ROM for save-type detection |
| Tests | Scenarios in C inside a test ROM (protothread-style macros, injected keys). mGBA scripting is Lua-only, so a 30-line Lua launcher remains, for screenshots and saving the log ([mGBA scripting](https://mgba.io/docs/scripting.html)) |
| Licenses | libtonc: MIT ([gbadev-org/libtonc](https://github.com/gbadev-org/libtonc)); some devkitPro headers derived from libgba carry LGPL notices ([tonc_libgba.h](https://github.com/devkitPro/libtonc/blob/master/include/tonc_libgba.h)). Maxmod: ISC ([devkitPro/maxmod](https://github.com/devkitPro/maxmod)) |

Date: 2026-09-24. Everything marked **(verified)** was checked on this machine, not just read about.

## 1. Summary of decisions

| Topic | Decision |
|---|---|
| Framework | **Butano 21.8.0** (C++ high-level GBA engine, released 2026-08-03), pinned as git submodule `external/butano` |
| Compiler / toolchain | **devkitARM r68** (GCC 16.1.0, binutils 2.46, newlib 4.6.0) from devkitPro pacman, group `gba-dev` |
| Build system | GNU Make (Butano `butano.mak`), wrapped by `build.ps1` / `build.sh` |
| Asset pipeline | Python 3 generator (`tools/`) → indexed BMP + JSON / WAV / MOD → Butano's grit + mmutil at build time |
| Audio | Maxmod (Butano default backend): music as ProTracker `.mod`, SFX as 8-bit `.wav` |
| Save | 32 KB SRAM through `bn::sram` (high score only) |
| Emulator / tests | mGBA. Stable 0.10.5 for play; **nightly (2026-09-19)** Qt build for `--script` Lua automation |
| Graphics mode | Mode 0 (4 regular tiled BGs) + 4bpp sprites, 1D mapping (Butano default) |

## 2. Toolchain research

### devkitPro / devkitARM
* Official path: graphical installer from GitHub (`devkitProUpdater-3.0.3.exe`), which installs an MSYS2 environment and uses **pacman** to install the `gba-dev` group. Sources: [Butano getting started](https://gvaliente.github.io/butano/getting_started.html), [devkitPro installer releases](https://github.com/devkitPro/installer/releases), [installer NSIS source](https://github.com/devkitPro/installer/blob/master/nsis/devkitPro.nsi).
* devkitpro.org (wiki) returned a Cloudflare block from this network; `pkg.devkitpro.org` (the pacman repos) was reachable. GitHub docs were used instead.
* **Discrepancy found:** running the installer silently (`/S`) sets `DEVKITPRO`/`DEVKITARM` and downloads MSYS2, but the NSIS script skips extraction when the GUI install page never ran (`$Install` flag). **(verified)** We reproduced the script's exact steps by hand: extract `msys-2.10.0.1.7z` to `C:\devkitPro`, patch `msys2/etc/fstab`, run `bash --login`, `pacman -Syu` (twice; the first restarts the core), then `pacman -S gba-dev`. In normal use, simply run the installer interactively and tick "GBA Development".
* Installed versions **(verified via `pacman -Q`)**: devkitARM r68-1, devkitarm-gcc 16.1.0, devkitarm-binutils 2.46.0, newlib 4.6.0.20260123, libgba 0.5.4, libtonc 1.4.5, maxmod-gba 1.0.15, mmutil 1.10.1, grit 0.10.0, gba-tools 1.2.0 (gbafix).

### Frameworks compared

| Option | Pros | Cons |
|---|---|---|
| libgba (devkitPro) | Thin register/BIOS wrappers, C | We would have to write OAM, VRAM allocator, palettes, text, and audio integration ourselves |
| libtonc | Excellent docs ([tonc](https://gbadev.net/tonc/)), C, low level | Same as above; more code = more bugs for a small team |
| **Butano** | Actively maintained (21.8.0, Aug 2026). Manages OAM, sprite tiles, VRAM, and palettes with sharing/dedup, fixed-point, no-heap containers, Maxmod audio, SRAM, text, a profiler and CPU-usage queries. Includes a complete shmup (*Butano Fighter*) | C++; requires Python; path without spaces |
| Wonderful Toolchain | Modern alternative | Butano support is still labelled experimental ([docs](https://gvaliente.github.io/butano/getting_started_wt.html)) |

**Choice: Butano on devkitARM.** It removes most hardware bookkeeping (OAM, VRAM allocation, palette sharing) so effort goes into the game. It is proven for exactly this genre (Butano Fighter, [Solar Guard](https://deft-spade.itch.io/solar-guard)). Every Butano API used in this project was checked against the 21.8.0 headers in `external/butano/butano/include`.

### Python
Butano's asset tools are Python scripts; the Makefile exposes a `PYTHON` variable. The Windows Store `python` alias is a stub, so the build passes the real interpreter path. On this machine that is found via `py`: `C:\Users\trant\AppData\Local\Programs\Python\Python312\python.exe` **(verified)**.

### Paths with spaces (important on this machine)
Butano's docs say: "put it in a path without spaces". **Verified** by building the template from `…\dkp\space test`: make fails with `No rule to make target '/home/…/space'`. The repository lives in `C:\Users\trant\Downloads\gba project` (space), so `build.ps1` temporarily maps the project to a free drive letter with `subst` and builds from `/X/`. **Verified** to produce a working ROM. `TARGET` is set explicitly so the ROM name does not depend on the directory.

### Linux / WSL
devkitPro distributes the same pacman packages for Linux ([devkitPro pacman](https://github.com/devkitPro/pacman)). `build.sh` works on Linux with `DEVKITARM` set. WSL is **not** needed on Windows: native MSYS2 from devkitPro is the officially supported path.

## 3. Hardware facts and constraints

Sources: [tonc – video](https://gbadev.net/tonc/video.html), [tonc – sprites](https://gbadev.net/tonc/regobj.html), [gbadoc memory map](https://gbadev.net/gbadoc/memory.html), [GBATEK mirror](https://mgba-emu.github.io/gbatek/). The problemkaputt.de original was unreachable.

| Item | Value |
|---|---|
| CPU | ARM7TDMI @ 16.78 MHz (2^24 Hz), ARM + Thumb; **no FPU** → fixed-point (`bn::fixed`, 12 fractional bits) |
| Screen | 240×160, 15-bit BGR color |
| Frame | 228 scanlines (160 draw + 68 VBlank) × 1232 cycles = **280,896 cycles/frame → 59.73 Hz** |
| BIOS | 16 KB |
| EWRAM | 256 KB, 16-bit bus, slow → large game-state objects |
| IWRAM | 32 KB, 32-bit, fast → stack, small globals, hot ARM code |
| Palette RAM | 1 KB = 256 BG colors + 256 OBJ colors (16 banks × 16 for 4bpp) |
| VRAM | 96 KB: 64 KB BG (charblocks 0-3), 32 KB OBJ tiles (charblocks 4-5) in tiled modes |
| OAM | 1 KB: **128 sprites**, 32 affine matrices |
| Sprite sizes | 8–64 px square/wide/tall (8×8 … 64×64) |
| Sprite per-scanline budget | 1210 cycles/line (954 with H-Blank-free bit). A normal sprite costs its width; an affine sprite costs `10 + 2·width`. **Consequence:** avoid rotated bullets; keep wide sprites rare |
| BG modes | 0: 4 regular tiled BGs; 1: 2 regular + 1 affine; 2: 2 affine; 3–5: bitmaps (only 16 KB OBJ VRAM) |
| DMA | 4 channels (DMA1/2 feed Direct Sound FIFOs) |
| Timers | 4 × 16-bit (Timer0/1 drive sample rate for audio) |
| Sound | 4 PSG channels (GB-compatible) + 2 Direct Sound 8-bit PCM FIFOs (A/B) |
| ROM | up to 32 MB, 16-bit bus, wait states |
| Save | SRAM 32 KB at 0x0E000000 (8-bit bus), or Flash/EEPROM |
| Input | 10 keys via REG_KEYINPUT (A, B, Select, Start, D-pad, L, R) |

**Mode choice:** Mode 0 gives 4 hardware-scrolled regular layers. That is ideal for horizontal parallax (far stars, nebula, terrain, HUD/effects), and it keeps the full 32 KB of OBJ VRAM. Affine BGs are unnecessary for a side-scroller.

## 4. Butano engine specifics (from 21.8.0 headers/docs)

* Main loop: `bn::core::init()` once, then `bn::core::update()` per frame (waits for VBlank, commits OAM/palettes/BGs). Source: `bn_core.h`.
* Perf metrics: `bn::core::last_cpu_usage()` (fraction of a frame), `last_missed_frames()`, `bn::memory::used_static_iwram()/used_static_ewram()/used_stack_iwram()`.
* Limits (`bn_config_*.h`): `BN_CFG_SPRITES_MAX_ITEMS 128`, `BN_CFG_SPRITE_TILES_MAX_ITEMS 128`, `BN_CFG_BGS_MAX_ITEMS 4`, audio mixing default 16 kHz, `BN_CFG_AUDIO_MAX_SOUND_CHANNELS 4`, `BN_CFG_AUDIO_MAX_MUSIC_CHANNELS 16`.
* Memory: globals are in IWRAM by default; `BN_DATA_EWRAM` / `BN_DATA_EWRAM_BSS` move data to EWRAM (`bn_hw_common.h`). The FAQ recommends placing scenes in EWRAM (Butano Fighter does this).
* The FAQ says to avoid the heap: Butano containers (`bn::vector`, `bn::optional`) are fixed-capacity and never allocate.
* Identical palettes are shared automatically. The first palette color is transparent.
* Graphics import: indexed **BMP without compression or color-space info**, 16 or 256 colors, plus a JSON descriptor (`"type": "sprite" | "regular_bg" | …`, `"height"`, `"bpp_mode"`). Source: [import guide](https://gvaliente.github.io/butano/import.html).
* Text: `bn::sprite_text_generator` + `bn::sprite_font`. A font needs **94 glyphs** starting at `'!'` (`bn_sprite_font.h` → `minimum_graphics = 94`; the generator maps `character - '!'`), 4bpp, uncompressed.
* Save: `bn::sram::read/write<T>()`; Butano embeds the `SRAM_V113` tag so emulators detect SRAM (`bn_hw_sram.bn_noflto.cpp`).

## 5. Audio research

* GBA has no hardware mixer for PCM. Direct Sound plays 8-bit samples through FIFOs refilled by DMA on timer overflow, so software (Maxmod) mixes channels. Modern formats (MP3/OGG) are not viable without a costly decoder.
* Butano + Maxmod accept `*.mod, *.xm, *.s3m, *.it` for music and uncompressed `*.wav` for SFX; recommended "8-bits 22050 Hz" ([import guide](https://gvaliente.github.io/butano/import.html)).
* **Decision:** generate original ProTracker `.mod` songs (4 channels, tiny synthesized waveform samples) and 8-bit WAV effects from Python. This is deterministic, original, and small.

## 6. Emulator / testing research

* **mGBA** ([mgba-emu/mgba](https://github.com/mgba-emu/mgba)): latest stable 0.10.5; accurate, has GDB stub, logging (Butano's log backend defaults to mGBA), and Lua scripting ([scripting API](https://mgba.io/docs/scripting.html)).
* 0.10.5 has no command-line way to run a script. **Verified:** the **nightly** Qt build (`mGBA-build-2026-09-19`) supports `--script FILE`; `mgba-sdl` does not. `QT_QPA_PLATFORM=offscreen` does not work in the Windows build, so a small window opens briefly during tests.
* **Verified** automation: a Lua script using `callbacks:add("frame", …)`, `emu:screenshot()`, `io.open`, and `os.exit()` ran a Butano ROM for 120 frames and saved a PNG. The same mechanism provides scripted input (`emu:setKeys`) and RAM inspection (`emu:read32`), so tests can assert game state.

## 7. Open-source references
* **Butano Fighter** (in `external/butano/games/butano-fighter`): vertical shmup; architecture reference (scenes in EWRAM, bullet pools).
* **Solar Guard** (GBA Jam 2021, GPL-3.0, Butano) — space shooter.
* tonc demos (libtonc) — low-level reference.
Only architecture ideas were taken; no code or assets were copied.

## 8. Risks

| Risk | Mitigation |
|---|---|
| Sprite scanline overflow with many bullets | Small 8×8 bullets, no affine bullets, hard pool caps |
| OAM exhaustion (128) | Budget table in architecture.md; pools sized to stay ≤ ~112 |
| IWRAM overflow | World/scene objects in EWRAM (`BN_DATA_EWRAM_BSS`) |
| Path with spaces breaks make | `subst` drive in build script (verified) |
| Maxmod choking on odd modules | Generator writes a conservative M.K. 4-channel MOD; tested in mGBA |
| Real hardware differences | No emulator-only features used; SRAM is standard; no misaligned DMA; untested on real hardware (see testing.md) |
