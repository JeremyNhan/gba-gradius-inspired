# Changelog

## v0.0.2-beta (2026-09-24)

* **Power ladder** replaces the six capsule types: every destroyed enemy has a chance to drop a P capsule (from 3 % for small asteroids to 100 % for gunships), and each capsule advances one step: normal shot → homing dot → missile → laser → shield → spread laser → shooter → 2 shooters → homing missile → shockwave. Losing a ship resets to the normal shot.
* New weapons: piercing laser and spread laser, homing dot, forward missiles, trailing additional shooters, periodic shockwave (clears enemies and bullets every 10 s, 0.5 s invulnerability).
* Fixed movement speed (the speed capsules are gone); extra-life capsule removed.
* Performance: sprites are built with `bn::sprite_builder`, bullet sprites are created at most 4 per frame, shooter volleys are staggered, and the HUD renders one text item per frame (worst frame 85 % with up to 32 player shots).
* Tests: new `power` suite (27 checks); full_run checks random drops and ladder progress (121 checks total).

## v0.0.1-beta (2026-09-24)

First public build.

* Complete game: title, 3 stages (Outer Rim, Crystal Caverns, The Dreadnought), 3 multi-phase bosses, stage clear, game over, ending.
* Normal and spread shot (3 levels each), homing missiles, charged piercing wave.
* Six power-up capsules: SPEED, SHOT, SPREAD, MISSILE, SHIELD, LIFE.
* Nine enemy types, including turrets, a gunship, mines and asteroids; scrolling terrain in stages 2 and 3.
* Generated MOD music and WAV sound effects.
* High score saved to cartridge SRAM.
* Automated mGBA test suite (94 checks), including a full playthrough.

Known limitations: tested in mGBA only (not yet on real hardware); shield and 1UP pickups are not covered by automated tests. See [docs/testing.md](docs/testing.md).
