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
| D-pad | Move (8 directions, 2 px/frame; diagonal speed normalised) |
| A (hold) | Fire the main gun; homing dots and missiles fire automatically once earned |
| B (hold, release) | Charge (45 frames) and release a piercing wave |
| START | Start / pause / resume |
| SELECT, L, R | Debug ROM only (see README) |

## Player

* 3 lives. Touching an enemy, a bullet, a boss or the terrain costs a life. There is no way to earn extra lives.
* After death: 90-frame respawn, then 150 frames of invulnerability (blinking).
* Death penalty: the power ladder resets to the normal shot (shield lost as well).

## Power ladder (data in `src/data/ss_power_data.h` and `src/data/ss_weapon_data.h`)

Every destroyed enemy has a chance to drop a **P** capsule (see the drop column in the enemy table). Each capsule collected moves the ship **one step up** the ladder. Powers are cumulative by slot: the main gun is upgraded in place, missiles become homing, everything else is added on top.

| Step | Power | Effect |
|---|---|---|
| 0 | Normal shot | Start: single fast bolt (7 px/frame, 2 damage) |
| 1 | Homing dot | + a small pellet every 20 frames that steers toward the nearest enemy (max 2 on screen) |
| 2 | Missile | + forward missiles angled down-forward (3 damage, max 2 on screen) |
| 3 | Laser | Main gun becomes a long laser that pierces enemies (stopped by bosses and terrain) |
| 4 | Shield | + 3-hit shield |
| 5 | Spread laser | Main gun fires three lasers (straight, up and down) |
| 6 | Shooter | + one additional shooter: a drone that follows the ship's path and fires the main gun |
| 7 | 2 shooters | + a second shooter |
| 8 | Homing missile | Missiles fire in pairs (up and down) and home in (max 4 on screen) |
| 9 | Shockwave | Fires immediately, then every 10 s: destroys all enemies and bullets on screen (with score), takes 10 % of the boss's max HP, 0.5 s invulnerability, white screen flash |

* A capsule gives 200 points. At step 9, further capsules refill the shield and give 1000 points ("FULL POWER").
* The HUD bottom line shows the loadout, e.g. `P9 S.LASER x3 DOT HMSL SHLD3 WAVE` (step, gun, gun count including shooters, extras).
* Drops use the world's seeded random generator, so a given play (same inputs) produces the same drops.
* Charged wave (B): 12 damage, passes through enemies and terrain. It is available at every step.

## Enemies (data in `src/game/ss_enemies.cpp`)

| Enemy | Plan role | Behaviour | HP | Score | Drop |
|---|---|---|---|---|---|
| Dart | A: basic fighter | Straight flight, aimed shot (stage 2+) | 2 | 100 | 8 % |
| Waver | B: sinusoidal | Sine wave, straight shots | 2 | 150 | 8 % |
| Interceptor | C: fast | Rushes in, stops, fires a 3-shot burst, dashes | 3 | 250 | 12 % |
| Turret | D: turret | Mounted on floor or ceiling, barrel tracks the player | 5 | 300 | 15 % |
| Gunship (Hulk) | E: armoured | Large, hovers, 3-way aimed spread | 40 | 2000 | 100 % |
| Swarm drone | F: formation | Flies loops in formation | 1 | 100 | 5 % |
| Mine | G: hazard | Drifts; bursts into an 8-way bullet ring when destroyed | 4 | 200 | 10 % |
| Asteroid (small/big) | G: hazard | Tumbling debris; big ones split into small ones | 5 / 18 | 50 / 400 | 3 % / 25 % |

Destroying every member of a bonus formation gives +500. Enemies wiped out by a shockwave give their score but never drop capsules.

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
* 4-channel MOD music (title, 3 stages, boss, ending, stage-clear and game-over jingles) and 8-bit WAV sound effects (shots, missile, charge-ready, beam, hits, shield hit, explosions, player death, pickup, full-power jingle, warning siren, menu select, pause), all generated by `tools/`.
