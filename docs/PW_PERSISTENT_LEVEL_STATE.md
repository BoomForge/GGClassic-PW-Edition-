# PW Edition — Player and RPG state across connected levels

**Status:** implemented in the development branch on 21 November 2026; **full-engine compilation and gameplay testing deliberately deferred** until a Windows workstation is available. This is not a claim that a complete RPG inventory system or disk-save system has been validated.

## What Classic already provided

GameGuru Classic's `scriptbank/global.lua` already defines `GameLoopSaveStats()` and `GameLoopLoadStats()` for its process-restart level mode (`allowfragmentation=2`). They save **health, lives, current weapon and ten weapon slots with ammunition, clips and ammo pools** to `savegames\\standalonelevelstats.dat`.

Ordinary in-process single-player level jumps (`allowfragmentation=1`) previously did not call this save/restore pair. The PW Edition engine now offers an opt-in path that does, **without inventing a new weapon save format**.

## Enable cross-level player state

In the environment used to launch a compiled standalone PW Edition game:

```powershell
$env:GGCPW_KEEP_LEVEL_STATS = "1"
./YourGame.exe
```

The flag applies only to standalone, single-player, in-process scripted `JumpToLevel` transitions. It does not change the editor, multiplayer or the original process-restart handling. Until proper testing, the flag is **off by default**.

### Transition behaviour

1. After a valid `JumpToLevel`, the engine records health and lives in process-local memory, removes any old per-level stats file when permitted and invokes the existing Lua `GameLoopSaveStats()` **before** `lua_quitting()` and level cleanup.
2. The next level starts normally, including weapon and player creation.
3. The engine invokes `GameLoopLoadStats()` only when a fresh stats file was created. It then reapplies the in-memory health/lives values as a fallback.
4. The pending snapshot is cleared after the restore so it cannot be inadvertently reused for the following unrelated level.

The previously existing process-restart path retains its behaviour. If the stats file cannot be refreshed, health/lives can still cross a normal in-process jump; weapon slots and ammo cannot be guaranteed in that case.

### Defensive Lua save-file handling

The modified `global.lua` keeps the original `v1` stats format and 43 numeric values. The reader parses **all 43 values before applying any**: a truncated or malformed file cannot partially overwrite player health and weapons. The new optional quest-state module is loaded with a protected `pcall(require, ...)`; if it is missing, ordinary Classic stats work as before.

## RPG values: quest flags, collected-item counters and decisions

A new module `scriptbank/pw_transition_state.lua` provides three global functions in the modified `global.lua`:

```lua
-- Example: completing a guild quest in one level
PWSetTransitionValue("guild.first_quest_done", true)
PWSetTransitionValue("merravale.gold", 42)
PWSetTransitionValue("story.faction", "rangers")

-- Example: a script in the next area
if PWGetTransitionValue("guild.first_quest_done", false) then
  -- quest is already complete
end
local gold = PWGetTransitionValue("merravale.gold", 0)

-- Clear all PW state when deliberately starting a completely new adventure
PWClearTransitionValues()
```

The values are held in memory while a level runs, then written to `savegames\\pw_transition_state.dat` by the normal GameLoop save hook. They are reloaded at the next level's GameLoop load hook.

**Limits:** maximum 128 keys, keys up to 64 alphanumeric/dot/underscore/dash characters, values restricted to booleans, finite numbers or strings up to 512 bytes. The state file stores hex-encoded values and a required end marker. It never executes source code from a saved file and rejects a truncated or invalid file rather than partially applying it.

This is **developer-owned RPG data**, not an automatic inventory system. If the game implements custom inventory or quest systems, its Lua scripts must explicitly set/read the relevant values. Existing GameGuru weapon slots are handled separately by the original Classic stats functions.

## Deployment and packaging

Source files:
- `GameGuru Core/GameGuru/Source/M-Game.cpp` — capture/restore around in-process level jumps.
- `GameGuru Core/GameGuru/Shaders and Scripts/scriptbank/global.lua` — defensive Classic v1 stats functions and new PW value wrappers.
- `GameGuru Core/GameGuru/Shaders and Scripts/scriptbank/pw_transition_state.lua` — limited Lua value serializer.
- `GameGuru Core/GameGuru/Source/M-MapFile.cpp` — includes the companion module in applicable standalone exports when present.
- `GameGuru Core/Guru-MapEditor/DarkEXE.vcxproj` — x64 build copies the two Lua files into the built editor's `Files/scriptbank` directory.

The updater ZIP allowlist and Python release packager now allow exactly `Files/scriptbank/global.lua` and `Files/scriptbank/pw_transition_state.lua` as **explicitly selected** supporting files. They are **not automatically included in a release**. A future properly authorised build must include both when publishing a version that uses this feature:

```powershell
python tools/make_pw_release.py --build-dir "D:\GGClassic-PW-Build" --version 0.1.0 --out-dir dist --include "Files/scriptbank/global.lua" --include "Files/scriptbank/pw_transition_state.lua"
```

Do not publish any modified GameGuru script/media without confirming the applicable source and redistribution licence.

## Deferred acceptance — do not mark as gameplay-tested

- [ ] Compile the full 64-bit editor, with installed licensed GameGuru media.
- [ ] Confirm the modified `global.lua` and companion module are the scripts actually executed in exported standalones.
- [ ] Jump 25+ times between town/interior/dungeon levels with the flag off and on.
- [ ] Carry health, lives and all ten weapon/ammo slots; test empty slots and missing weapons in destination levels.
- [ ] Carry booleans, numbers and strings used by quest/item systems.
- [ ] Test missing module, missing save directory, truncated/corrupt files and disk write failure.
- [ ] Verify initial new game and returning to title do not accidentally load stale prior stats.
- [ ] Check process-restart mode (`allowfragmentation=2`), save slots, standalone packaging, script initialisation order and multiplayer regressions.
- [ ] Confirm any persistent RPG quest values required during an entity's `_init` are loaded early enough; current restore occurs later in the level preparation path.
- [ ] Only then decide whether to enable ordinary jump persistence by default.

**This upgrade does not yet implement persistent saves across unrelated game launches, seamless area streaming, or full quest/inventory systems.**
