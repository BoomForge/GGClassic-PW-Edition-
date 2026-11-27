# PW Edition — Persistent player profiles M1

**Status:** source implemented; isolated Windows CI and live standalone validation pending.

In a single-player exported game using normal in-process level transitions (allowfragmentation=1):

```lua
PWSetTransitionValue("quest.guild_done", true)
PWSetTransitionValue("inventory.gold", 42)
local saved = PWProfileSave("hero")
-- The same file is refreshed by developer-triggered checkpoint/autosave calls.
PWProfileCheckpoint("hero")
PWProfileAutosave("hero")
local names = PWProfileList()
-- Request load once in response to a user action; not every frame.
PWProfileLoad("hero")
```

Save returns a boolean. Load returns whether the name was accepted for dispatch, not completion. Poll `g_PWProfileStatus` for the engine's result. A successful load reloads the saved FPM and applies player state; only explicitly registered scalars survive, not arbitrary world state. Profile names: 1–32 ASCII letters/digits/dashes/underscores; case insensitive. First save creates a profile. Files: `Files/savegames/pw_profiles/<name>.pwprofile`, plus a previous valid `.bak` for recovery. No user data is executed as Lua source. A checksum detects accidental corruption (not authentication). The engine validates the target FPM and full Lua payload before unloading the current level. Recovery falls back to the prior valid envelope if the main file is corrupt.

M1 intentionally does not add a default menu, rename/delete, timed autosaves, process-restart mode, editor or multiplayer support, automatic entity/world serialization or stable weapon-name remapping across different level weapon banks. Checkpoint/autosave are explicit APIs. The existing game files and transitions are unchanged unless a script invokes the API. Both modified Lua scripts must be installed/exported with the engine.

## Acceptance
- Verify full Windows x64 compilation and the isolated level-flow/profile CI tests.
- In two licensed standalone maps, save two distinct profiles with different health, lives, weapon slots, ammo, location, camera angle, boolean/number/string flags.
- Quit/restart and request a load from the running first map; confirm the correct saved level and player state, including values needed in entity initialization.
- Save and reload the same map; test absent level, unknown profile, malformed and truncated file, recovery after corrupt primary, a save after recovery, invalid names, and interrupted writes.
- Confirm ordinary level jumps, legacy save slots and new game are unaffected, and existing multi-grass remains working.
- Follow-up: profile-management screen (create/select/list/rename/delete), automatic checkpoint policy, stable weapon identity mapping and live tests.
