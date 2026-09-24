# Game Design

Space Shooter is an original horizontal shoot-'em-up for the Game Boy Advance. It follows the structure of classic 16-bit console shooters: a ship on an auto-scrolling stage, capsule upgrades, enemy waves, terrain, and a multi-phase boss at the end of each stage. All names, art, music, level layouts and enemy designs are original.

## Flow

```
TITLE → STAGE 1 "OUTER RIM" → boss WARDEN → STAGE CLEAR
      → STAGE 2 "CRYSTAL CAVERNS" → boss HIVE → STAGE CLEAR
      → STAGE 3 "THE DREADNOUGHT" → final boss OVERMIND → ENDING → TITLE
Any stage: last life lost → GAME OVER → TITLE
```

Each stage is about 80 seconds of scripted waves (a data table of frame-stamped events in `src/data/ss_stage_data.cpp`), then a WARNING banner with a siren, then the boss.

| Stage | Setting | Terrain | Hazards |
|---|---|---|---|
| 1 Outer Rim | nebula | none (open space) | asteroids (big ones split), first armoured gunship |
| 2 Crystal Caverns | cave backdrop | crystal ceiling and floor, narrowing | ceiling/floor turrets, mine fields |
| 3 The Dreadnought | battleship hull | metal ceiling/floor | dense turrets, gunships, all enemy types |

Difficulty scales by stage: enemy fire intervals get shorter, and darts only start shooting from stage 2.

## Controls

| Button | Action |
|---|---|
| D-pad | Move (8 directions; diagonal speed normalised) |
| A (hold) | Fire the primary weapon; missiles fire automatically once collected |
| B (hold, release) | Charge (45 frames) and release a piercing wave |
| START | Start / pause / resume |
| SELECT, L, R | Debug ROM only (see README) |

## Player

* 3 lives (max 9). Touching an enemy, a bullet, a boss or the terrain costs a life.
* After death: 90-frame respawn, then 150 frames of invulnerability (blinking).
* Death penalty: shield lost; weapon level, missile level and speed each drop by one step (never below the base level).
* Shield: absorbs 3 hits.

## Weapons (data in `src/data/ss_weapon_data.h`)

| Weapon | HUD | Levels 1 → 3 |
|---|---|---|
| Normal shot | `SHOT` | 1 → 2 → 3 parallel fast bolts (7 px/frame) |
| Spread shot | `WIDE` | 3 → 5 → 5 stronger pellets in a fan, slower |
| Homing missiles (secondary) | `MSL` | Level 1: single missiles (up to 2 on screen); level 2: pairs (up to 4). They turn toward the nearest target |
| Charged wave | — | Hold B; 12 damage, passes through enemies and terrain |

## Power-ups

Carrier enemies (and completed carrier formations) drop a capsule. Drops follow a fixed rotation (SHOT, MISSILE, SPEED, SPREAD, SHIELD, …, LIFE), so a given play always produces the same drops. Capsules for upgrades that are already maxed are skipped.

| Capsule | Effect |
|---|---|
| SHOT | Switch to normal shot, or +1 level if already equipped |
| SPREAD | Switch to spread shot, or +1 level |
| MISSILE | +1 missile level (max 2) |
| SPEED | +1 speed level (1.5 / 2.0 / 2.5 px per frame) |
| SHIELD | Full 3-hit shield |
| LIFE | +1 life |

Picking up a capsule gives 200 points, or 1000 if it upgraded nothing.

## Enemies (data in `src/game/ss_enemies.cpp`)

| Enemy | Plan role | Behaviour | HP | Score |
|---|---|---|---|---|
| Dart | A: basic fighter | Straight flight, aimed shot (stage 2+) | 2 | 100 |
| Waver | B: sinusoidal | Sine wave, straight shots | 2 | 150 |
| Interceptor | C: fast | Rushes in, stops, fires a 3-shot burst, dashes | 3 | 250 |
| Turret | D: turret | Mounted on floor or ceiling, barrel tracks the player | 5 | 300 |
| Gunship (Hulk) | E: armoured | Large, hovers, 3-way aimed spread | 40 | 2000 |
| Swarm drone | F: formation | Flies loops in formation | 1 | 100 |
| Mine | G: hazard | Drifts; bursts into an 8-way bullet ring when destroyed | 4 | 200 |
| Asteroid (small/big) | G: hazard | Tumbling debris; big ones split into small ones | 5 / 18 | 50 / 400 |

Formations: line, V, column, wave, swarm loop, mine field.

## Bosses (`src/game/ss_boss.cpp`)

Each boss has 3 HP-based phases, flashes white when hit, and has a boss bar at the top of the screen. It dies in a multi-explosion sequence that also cancels its remaining bullets. Behaviour depends only on the boss timer, the phase and the player position, so it is deterministic.

| Boss | Stage | Phases |
|---|---|---|
| WARDEN | 1 | Bobbing gun platform. Aimed volleys; faster movement per phase; phase 3 adds a rotating bullet stream and horizontal sway |
| HIVE | 2 | Phase 1: rotating spiral arms. Phase 2: ram cycle (aligns with the player, telegraphs, dashes left, returns, ring burst). Phase 3: spiral arms return |
| OVERMIND | 3 | Armoured (half damage) while its eye is closed in phase 1; guarded by pods in front of it. Needle waves from the eye and big rings, becoming denser per phase |

## Scoring and save

Score comes from enemies, bosses and capsules. The high score is stored in cartridge SRAM (with a magic value and checksum; invalid data falls back to a default of 20,000) and shown on the title screen.

## Presentation

* Mode 0 with 3 scrolling layers: far starfield (×0.25), stage backdrop (×0.5), terrain (×1). The title uses the fourth layer for the logo.
* One 16-colour master sprite palette plus a boss palette: high-contrast 16-bit style.
* Hit flashes, explosions and sparks, screen shake on big explosions and player hits, palette fades between screens.
* 4-channel MOD music (title, 3 stages, boss, ending, stage-clear and game-over jingles) and 8-bit WAV sound effects (shots, missile, charge-ready, beam, hits, shield hit, explosions, player death, pickup, 1UP, warning siren, menu select, pause), all generated by `tools/`.
