# Pokemon Purple — Flipper Zero

Pokemon Purple is a Flipper Zero monster-RPG project.

## v0.4 engine migration

This pass replaces the earlier prototype with a cleaner, Flipper-Mon-inspired architecture:

- scene-based title / starter / exploration / battle flow
- camera-following 20x20 tile map
- 16x16 bitmap trainer and tiles
- random grass encounters
- battle states and move selection
- all 151 Gen 1 species enumerated and named in the data layer
- starter stats/moves plus initial wild-species data
- hold BACK from anywhere to exit cleanly

The full 151 are **not all fully implemented yet**. The roster/data layer now knows all 151 names, but most species still use generic fallback stats and generic visuals until their individual data/art is added.

## Upstream inspiration / attribution

The architecture and direction are based on the GPLv3 project Flipper Mon:
https://github.com/purplefox-io/flipper-mon

Flipper Mon is © its respective contributors and licensed under GNU GPL v3.
This project retains GPLv3 licensing for derivative work.

## Build

Use uFBT:

```sh
ufbt
```

Or use the included GitHub Actions workflow.
