# Pokemon Purple — Flipper Edition v1.0

A native Flipper Zero creature RPG built around a unified Gen 1 roster.

## Included
- All 151 Gen 1 species in the in-game dex/name table
- All 151 obtainable in one save (no version exclusives / trade requirement)
- Starter choice
- 32x14 overworld with 8 progression zones
- Random encounters
- Catching
- 6-mon party
- Party leader swapping
- HP, XP and leveling
- Potions and balls
- Save/load to SD card
- Dex: seen / caught tracking
- Healing centers
- 8 gym progression loop
- Champion state
- Post-game legendary spawning in Zone 8
- Mewtwo and Mew included in the post-game pool

## Controls
### World
- D-pad: move
- OK: interact (Center/Gym) or open Party
- Back: Pokédex

### Battle
- OK: battle menu
- Up/Down: choose action
- Back: close menu
- Actions: Attack / Catch / Potion / Run

### Party
- Up/Down: choose Pokémon
- OK: make selected Pokémon the lead
- Back: return

### Pokédex
- Left/Right: pages
- OK/Back: return

## Build
With uFBT:
```
ufbt
```
The FAP will be produced in `dist/`.

## Save path
`/ext/apps_data/pokemon_purple/save.bin`

## Notes
This is an original Flipper-native RPG implementation, not a Game Boy ROM or emulator.
The screen/UI and battle system are intentionally simplified for the Flipper Zero.
