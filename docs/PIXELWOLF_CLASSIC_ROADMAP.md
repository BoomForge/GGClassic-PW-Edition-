# GGClassic PW Edition — Community integration and remaining issues

**Reviewed:** 2026-11-21 (Australia/Hobart)  
**Owner:** BoomForge  
**Source:** https://github.com/Dark-Basic-Software-Limited/GameGuruRepo  
**Fork:** https://github.com/BoomForge/GGClassic-PW-Edition-  
**Upstream baseline:** `821f688580821af64c8371f2b1f6e760b1c68e4f` (2026-08-27)  
**Working branch:** `work/pw-community-integration-20261121`

## Goal

Keep the familiar GameGuru Classic first-person visual world-building workflow while preparing a low-overhead, extensible foundation for *Adventuring for Dummies*. Start with a very small early-2000s-style 3D world, **not** a complete RPG. Avoid heavy engine rewrites and protect editor/test-play usability.

## Community contributions reviewed

The fork began **identical to upstream master** (comparison returned 0 commits ahead/behind).

- `plemsoft:master`: 0 commits ahead; performance work already incorporated into upstream.
- `cybernescence:master`: 0 ahead; older contributions are included upstream.
- `KronosInfraction:master`: 0 ahead; older than upstream.
- `RednibCoding:master`: 0 ahead.
- `Teabone3:master`: 1 ahead, but an unmerged example/script addition, not a loading or importer fix.
- Upstream PR #6261 (Lafette shader, submitted for integration): **staged in this branch** as `apbr_animwithtran_emissive.fx`. Supports an animated PBR material variant with alpha and emissive textures. Based on existing `apbr_core.fx` flags. **Not yet compiled or tested in-game.**
- Upstream PRs #6351 (LUT zone example), #6344 (unskippable video zone), #6196/#6141/#6138 (Lua comments/doc additions): **not imported**. These are not fixes for our immediate priorities, and some have behaviour or documentation risks.

### Validation status

**Verified:** fork is identical to upstream before branch edits; upstream shaders use the `ILLUMINATIONMAP`, `WITHANIMATION` and `ALPHACLIP` macros; staged shader matches the upstream PR.

**Not verified:** Windows compilation, shader runtime compilation, editor startup, faster loading, modified asset import, real-time shadow quality, or project compatibility. Do not call this a working or improved binary until tested on Windows.

## PNG textures alongside DDS — source changes staged 2026-11-21

The user's requirement is **add PNG, keep DDS**. Existing Classic image loaders already handle PNG in some contexts; the missing support involves format-specific model/material lookup and packaging.

Implemented on the current development branch (not yet compiled or tested):
- `GameGuru Core/GameGuru/Source/M-Entity.cpp`: DDS-first fallback to same-name PNG for entity base maps and companion normal, specular/metalness, AO, gloss, height, emissive/illumination, detail texture lookups; _color.png multi-material override companion handling.
- `GameGuru Core/GameGuru/Source/M-Importer.cpp`: FBX converted-texture lookup tries existing texture names first, then a same-name DDS if present, and otherwise a same-name PNG.
- `GameGuru Core/GameGuru/Source/M-MapFile.cpp`: adds PNG _D/_N/_S files to the applicable level/archive file list without removing DDS entries.
- Root `README.md` now contains **Completed / In progress / To do** upgrade tracking, including status distinctions.

**Acceptance still open:** Build with VS2022; load PNG and DDS sets for one static Blender model and one rigged model; verify importer previews, material stages, map archive save/reopen and standalone packaging; check no DDS regressions. Other specialised texture formats (lightmaps, terrain generated textures and legacy character-creator internals) remain format-specific until explicitly validated. Do not claim universal PNG support yet.

## Test Level loading profiler — source and CI completed (2026-11-21)

The next performance phase is **measure before changing behaviour**, especially before skipping terrain work or removing save-before-preview safeguards. Relevant evidence: `editor_previewmapormultiplayer_initcode` performs **1,050,625 terrain-height queries** when terrain is present. This is a possible bottleneck, not a measured conclusion.

**Implemented in the development branch:** a Windows QueryPerformanceCounter RAII profiler (`PW-LoadTrace.h`) activated only by `GGCPW_PROFILE_LOAD=1`, integrated with Test Level preview initialisation, test-map save, terrain-height sampling, heightmap generation, environment reload, entity bank, level data, AI, weapon, HUD, script and media load phases. CSV is written to the user's Windows temp directory. A Python analysis tool and report tests were added, with [benchmark instructions](docs/PW_LOADING_PROFILE.md).

**Verified:** [Windows CI run #44817890932](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44817890932) passed Python report tests, isolated VS2022 x64 compilation of the profiler header, and end-to-end generation and analysis of a synthetic timing CSV. The GitHub source commits were independently read back.

**Not verified:** compiling the full GameGuru executable, actual Test Level timing in a licensed PW Edition installation, or any observed reduction in load time.

**Next:** compile the engine, profile repeated empty and populated Test Level runs, rank expensive stages and then implement one targeted performance change with before/after timing and regression checks. Do not present this as an optimisation completed until a measured improvement exists.

## Remaining issues in priority order

| Priority | Area | Issue / evidence | Acceptance goal |
| --- | --- | --- | --- |
| P0 | Long editor-to-test/load times | [Upstream #3100](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/3100) | Benchmark cold launch, editor-to-test and repeated test of an empty map and a small populated map. Profile before altering the loader. Shorten repeat tests without breaking media or standalones. |
| P0 | Rigged/animated FBX imports | [Upstream #1894](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/1894) | Import a known Mixamo FBX character with materials and animations, save/reopen, test animation and export standalone. |
| P0 | Imported textures/materials | [Upstream #4659](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/4659) | Consistent Blender asset import, material paths, texture overrides, save/reopen. |
| P0 | Build and regression baseline | [Upstream README](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/blob/master/README.md) | Use VS2022 Desktop C++ + MFC and required purchased-product media. Record baseline build and reproducible small-map smoke tests before riskier changes. |
| P1 | Modern GLB/glTF assets | New enhancement; no verified built-in GLB pipeline | Evaluate safe Assimp/conversion option, preserve skinning and animation; do not assume a converter alone solves GG character rigs. |
| P1 | Dynamic local-light shadows | [Upstream #555](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/555) | Verify existing real-time directional sunlight shadows. Prototype shadows cast by movable/local lights separately with performance limits. |
| P1 | Terrain performance | [Upstream #6223](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6223) | Profile before changing terrain renderer; retain early-2000s visual target and low-spec GPU performance. |
| P1 | 64-bit culling | [Upstream #5643](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/5643) | Verify actual 64-bit visibility/culling and compare GPU draw calls in a repeatable scene. |
| P1 | Incorrect dynamic lighting/normal maps | [Upstream #6209](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6209) | Local lights should preserve material normal detail under test conditions. |
| P1 | Waypoint editing regression | [Upstream #6444](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6444) | Waypoints can be added, moved, saved, and used by a test NPC. |
| P2 | Shadow and lighting quality | [Upstream #1192](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/1192) | Lighting settings predictable; acceptable defaults for PS2/early-2000s look. |
| P2 | Independent shader contribution | [Upstream PR #6261](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/pull/6261) | Compile shader, render animated transparent/emissive entity, ensure no regression on non-animated models. |

## General-purpose game-system requests — 22 November 2026

**Provenance:** Suggestions supplied by the user from another GameGuru Classic creator's off-the-cuff feedback. These are cross-genre engine improvement requests, not features of any particular Pixelwolf game. **Status: requested / not implemented.** Design specifics below are proposed engineering acceptance criteria, not a claim of existing Classic behavior.

### P0 — Reliable player-profile save/load system (new persistent save subsystem)

**25 November M1 source progress:** Named versioned atomic saves with one recovery copy, Lua save/checkpoint/autosave/list/load API, resolved level, player transform, health/lives, ten weapon/ammo slots and explicit scalar state. In-process standalone load integration staged. See [profile API](PW_PLAYER_PROFILES.md). Menu, rename/delete, automatic scheduling, cross-restart acceptance and full engine tests remain.

**User request:** An older-console-style profile created by the player, where autosave, checkpoint and manual save record all relevant character/world progress in one save file associated with that profile; subsequent sessions read that state when the player loads the profile.

**Important scope distinction:** Current `GGCPW_KEEP_LEVEL_STATS=1` and `PWSetTransitionValue` / `PWGetTransitionValue` support **limited in-process level transitions** and explicitly registered values. They are not full restart-safe per-player save files. Do not present the existing work as completing this request.

Acceptance criteria:
- [ ] Create, select, list, rename and delete player profiles with safe filesystem names and a clearly documented location; avoid using arbitrary user-entered paths directly.
- [ ] Support manual, checkpoint and autosave entry points for the active profile, with game-developer choices for when each fires. Keep the user's **single logical profile file** design simple; if internal checkpoints or recovery snapshots are needed, explain exactly how they coexist with this model.
- [ ] Store a versioned, schema-validated state snapshot: current/target level and player location/orientation; health/lives, weapon slots/ammunition; developer-registered typed Lua variables (such as quests, inventory or counters); and any other gameplay state **only where an explicit save/restore adapter exists**. Unknown arbitrary engine or Lua variables cannot safely be promised as automatically serialisable.
- [ ] Write transactionally (temp file, flush, atomic replacement where supported), detect corruption/incompatible versions, and retain a bounded recovery copy. Avoid partial writes and never apply a corrupt file over the active game's valid state.
- [ ] Restore state in a defined sequence before scripts/entities depend on it; support standalone builds, game restarts, level travel and migrated save versions while preserving old-game behavior when the new system is unused.
- [ ] Test save/quit/restart/load, missing/corrupt saves, interrupted writes, different profiles, autosave/checkpoint/manual flows, and two- or three-level projects.

### P0 — Non-explosive projectile collision and direct damage

**25 November source progress:** Corrected direct damage classification, frame-segment player collision, zero-motion ray guard and attacker attribution. Actual gameplay reproduction and regression acceptance remain open; no change to explicit no-damage projectile specs.

**User report:** Non-explosive arrows/bolts and other projectiles allegedly fail to lower the health of the player or active targets; explosive projectiles appear more reliable. This is a **reported bug, not yet reproduced in this fork**. The desired effect enables boomer shooters, dodgeable projectiles and bow-and-arrow combat.

Acceptance criteria:
- [ ] Reproduce with a controlled non-explosive projectile fired at the player and an AI/active entity, then compare equivalent explosive and hitscan damage paths.
- [ ] Trace projectile movement, collision flags, hit ownership/faction, entity damage handling, Lua notification and health updates. Identify exactly where direct collision damage is lost before changing code.
- [ ] On a valid impact, apply configured **direct-hit damage** to damageable targets regardless of whether explosion visuals/area damage are enabled. Do not accidentally double-apply damage for explosive projectiles, hit multiple times on one collision, or damage non-damageable objects.
- [ ] Preserve projectile travel, dodging, collision filtering, impact/disposal, optional friendly-fire and physics behavior. Verify bows/arrows, ballistic energy projectiles and gameplay script callbacks across editor Test Level and standalone builds.
- [ ] Cover player vs NPC/active object, collision vs near miss, obstruction/terrain hits, multiple fire rates, lethal hits and damage/armor scaling with deterministic tests.

### P1 — Player-configurable controls / key rebinding

**25 November M1 source progress:** Startup Controls dialog, 14 keyboard actions, persistent validated per-game bindings, duplicate detection and project/Classic reset. Native player and common Lua action queries use the separate mapping. Raw keys unchanged; mouse/controller rebinding and full in-game options remain open. Windows/live acceptance pending.

**User request:** A normal game's Options > Controls interface for player-defined controls, despite many existing GameGuru Lua scripts hard-coding their input keys. This affects general keyboard/mouse/controller gameplay, not just weapon binding.

Acceptance criteria:
- [ ] Introduce a central **gameplay action mapping** layer (move, jump, crouch, interact, fire, aim, reload, sprint, etc.) and a usable Controls menu with editable bindings, reset to defaults, conflict warnings and persisted settings per appropriate game/player scope.
- [ ] Keep game projects running when they still use legacy raw-key Lua queries; avoid silently remapping unrelated arbitrary keys or breaking typing/UI input.
- [ ] Offer documented Lua action-query APIs for new scripts and a staged compatibility/migration path for common stock scripts with hard-coded key codes; inventory and interaction scripts may require explicit updates.
- [ ] Test keyboard/mouse inputs first and investigate controller support separately where feasible. Validate pause/menu focus, alternate layouts, duplicate bindings, input after reloading and standalone export.
- [ ] Distinguish **input-key binding** from **weapon-to-character-bone/socket binding** already specified under Importer 2 / Character Creator 2. Neither is implemented by the existing cross-level weapon persistence code.

### P1 — Basic game display options (windowed/fullscreen and resolution)

**25 November M1 source progress:** Native startup choice, compatible windowed resolution list, borderless desktop fullscreen, saved preference, timed confirmation and interrupted-start recovery. New exports opt in via setup.ini; existing exports and VR retain Classic behaviour. Full Windows and standalone validation remain; exclusive fullscreen/secondary-monitor/in-game selection deferred.

**User request:** A straightforward options menu allowing the player to select **windowed or fullscreen** and choose a **screen resolution**. An uncomplicated settings window presented at game startup is an acceptable alternative to a complete in-game Options menu. This is a general feature for exported games of any genre.

Acceptance criteria:
- [ ] Offer windowed and fullscreen display modes, with a resolution selector populated from modes supported by the current display and renderer. Keep the minimum interface simple; additional settings are optional.
- [ ] Make the display settings accessible at startup **or** in-game (an initial launcher/configuration window is acceptable). If a game can change display mode during play, avoid destroying unsaved game progress.
- [ ] Apply requested changes safely, with confirmation and automatic fallback to the last known working configuration if a resolution is unsupported, times out or leaves the screen unusable.
- [ ] Persist selected display preferences between runs in a clearly documented, game-specific settings file; distinguish display preferences from player-profile save data.
- [ ] Validate on Windows x64 standalone builds: first run, repeat launch, desktop and laptop resolutions, multi-monitor changes, DPI scaling and switching windowed/fullscreen without breaking aspect ratio, mouse capture or GUI layout.
- [ ] Preserve Classic's current default graphics startup for older projects unless the new options UI is explicitly enabled or integrated with a backwards-compatible default.

**Status:** requested / not implemented. Whether to use a startup dialog or an in-game menu can be chosen during implementation, as long as players can change the minimum two settings.

### Highest active priority — Full native third-person gameplay (25 November 2026 owner request)

Move ahead of Model Importer 2. Native orbit camera and avatar facing source stage committed; full controller, animation, combat, editor setup and lifecycle remain to implement and validate. See [native third-person development plan](PW_NATIVE_THIRD_PERSON.md). Preserve the first-person controller and original script compatibility.

### Relative priority

1. Stabilise the current importer/editor integration and its existing regression gates.
2. Reproduce the non-explosive projectile damage bug before altering projectile internals.
3. Design the profile-save state schema and persistence adapters around existing transition values, then test crash-safe restart recovery.
4. Introduce basic player display options (windowed/fullscreen and resolution), with a safe recovery path.
5. Introduce configurable gameplay-action controls with explicit Lua compatibility safeguards.
6. Keep weapon bone/socket binding in the separate rigging roadmap and prevent it being confused with gameplay keybinding.

All four requests require source implementation, standalone tests and actual Windows gameplay acceptance. Their presence on the roadmap is not proof of a fix.

## Editor UX request — safely remove custom assets (22 November 2026)

**25 November source progress:** Recoverable FPE registration quarantine via bottom browser context menu and confirmation implemented. Restricts removal to ordinary custom user FPE paths, refuses current bank references and never deletes shared model/texture/script files. Cross-map dependency analysis, full runtime validation and optional in-editor recovery remain open.

**User observation:** After the `SerineTest` FBX import produced a green thumbnail and an unusable entity, the owner manually deleted the imported files because there is no convenient in-editor asset deletion. This should be fixed independently of the FBX import defect.

- [ ] Add **Delete imported asset…** to the bottom Asset Browser for verified user-created entities under `Files/entitybank/user/`; do not allow deleting original built-in, Steam, DLC, purchased or shared asset packs through this option.
- [ ] Show a confirmation dialog with the exact entity and planned file operations, offer safe recovery (Recycle Bin or undoable quarantine), and never remove arbitrary files using unvalidated relative paths.
- [ ] Refuse to delete entities referenced by the current scene; ideally check all known map dependencies. Do not remove shared textures or files still referenced by other entities.
- [ ] After safe deletion, remove stale thumbnails/selection entries and refresh the browser without requiring editor restart; test cancellation, deleted/missing files, active scene references and large folders.
- [ ] Treat unsuccessful FBX/GLB imports as failed operations, with diagnostic messages, rather than publishing ghost model entries that require manual clean-up.

**Status:** tracked, not implemented. The separate imported-geometry validation is implemented in source and its automated tests pass, but in-editor deletion and local runtime acceptance remain pending.

## Implementation rules

1. Protect the upstream-compatible `master` branch. Use separate branches and PRs for improvements.
2. No bulk cherry-picking from older forks: the performance contributions found are already on upstream `master`.
3. Do not change core loading or importers without instrumented before/after measurements and a reproducible sample asset/level.
4. Keep work cumulative, with per-feature smoke tests and clear compatibility notes.
5. Distinguish **source change**, **build success**, **runtime validation**, and **user acceptance**. They are not equivalent.
6. Never add downloaded proprietary GameGuru media to this public repository.

## Licence and build prerequisites

This code is **source available but not open source**. It remains copyright The Game Creators Ltd. Consult the official repository's licence terms before distributing modified binaries. Per the upstream README, compilation needs Visual Studio 2022 Community with Desktop C++ and optional MFC components, as well as the game media owned through the product. The large repo media is distributed separately by the publisher.
