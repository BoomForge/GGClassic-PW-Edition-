# GGClassic PW Edition — Standalone level transition hardening

**Status: Source integrated on the development branch; actual full GameGuru build and multi-level runtime benchmarks NOT YET performed.** These are conservative foundations, not a new streaming world engine.

## Change 1 — Safer level transitions (active in source)

- Scripted standalone `JumpToLevel` now validates the destination **before** setting the level-ending fade or tearing down the current game state.
- Rejects missing, zero-byte or unsafe level names (absolute paths, traversal and reserved Windows path characters) and leaves the current level running. Displays a Lua prompt and writes a diagnostic timestamp.
- Supports existing `Files/mapbank/<level>.fpm` and one nested folder such as `Files/mapbank/Interiors/tavern.fpm`, matching the engine's previous search scope.
- Multiplayer, initial-title scene loading and the original save-game flow still use their existing paths; their complete validation remains future work.
- Adds opt-in timing for `game.transition.cleanup`, `game.transition.next_level_init` and `game.transition.fpm_load.total` to the previously implemented profiling CSV.

**Further upgrade staged:** opt-in continuity for health, lives, Classic weapon/ammo slots and explicitly managed Lua quest flags is now implemented in source. See [PW persistent level state](PW_PERSISTENT_LEVEL_STATE.md). It is untested and does not implement a full RPG inventory, general save-game system, renderer/device-loss recovery, crash recovery or atomic scene swaps.

## Change 2 — Avoid redundant level lookup and extraction work

Two complementary mechanisms:

1. A **per-process mapbank path lookup cache**, which avoids repeating Classic's folder enumeration for already resolved levels. A cached location is always checked for existence; changed/missing files fall back to a fresh lookup. This is active with the new scripted destination validation.
2. An **opt-in per-process .fpm extraction cache**, which snapshots the archive's extracted files into a private Windows temporary folder. On revisiting the same unchanged archive, the engine may copy the verified cached files back into `levelbank/testmap` instead of decompressing the .fpm again. It checks the archive's size and modified timestamp, verifies the cached file list and file metadata and falls back to the original extraction path if anything differs or cannot be copied.

The second mechanism is **OFF by default** until in-engine testing confirms compatibility with levelbank, lightmaps, terrain, AI obstacles and custom entity files. No cached data is used for an edited FPM whose recorded size or timestamp changes. No original .fpm is modified.

### Enable experimental FPM extraction caching

From PowerShell before launching the compiled PW Edition standalone game:

    $env:GGCPW_FPM_CACHE="1"
    ./YourGame.exe

Without that variable, GameGuru's normal original extraction path runs.

## Change 3 — Prepare connected areas before their loading screen

- The existing scripted `JumpToLevel` event now optionally starts a **low-priority background read** of the destination .fpm (maximum 64 MiB) while Classic's existing end-of-level fade is still running.
- The background worker uses only a separately owned file path and Win32 file I/O. It never accesses GameGuru graphics, scripting, editor state or physics from its worker thread.
- Reading may warm the Windows filesystem cache for the subsequent archive extraction. The worker does not keep a second live level in memory or build meshes off-thread.
- Optional FPM extraction caching can help on **return trips** to a previously loaded area.

This is an incremental connected-area loading foundation, **not seamless open-world streaming** and not a guaranteed measurable improvement. Actual gain depends on disk, FPM size, operating-system caching and how long the fade lasts.

### Enable experimental area prefetch

    $env:GGCPW_PREFETCH_LEVELS="1"
    ./YourGame.exe

Both experimental switches can be set together when profiling. Neither is enabled automatically in normal installs.

## Tests

The isolated x64 C++ contract test under `tools/test_pw_level_flow_win.cpp` creates synthetic FPM-named files and covers:

- existing direct and one-subfolder destinations;
- rejection of absolute, unsafe and missing targets;
- invalidation of stale resolved paths;
- opt-in bounded background archive reading;
- disabled-by-default FPM extraction cache;
- save/restore of synthetic cached files;
- invalidation when the original FPM file changes;
- basic cache path-safety rejection.

GitHub workflow: [PW Edition level transitions validation](../.github/workflows/pw-level-transitions-validation.yml).

**Verified:** [Windows x64 CI run #44821335693](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44821335693) completed successfully. It compiled the isolated C++14 helpers and ran tests for missing and unsafe level requests, stale and newly preferred mapbank paths, corrupted FPM cache entries, archive modification and asynchronous prefetching.

**Crucial limitation:** These synthetic tests do not compile `M-Game.cpp` or `M-MapFile.cpp` as part of the full engine and do not prove compatibility with real GameGuru FPM maps.

## Required real-world acceptance

- [ ] Build the full 64-bit editor and standalone game in Visual Studio 2022 with legally acquired media.
- [ ] Visit a tiny outdoor level, a tavern interior and a cave; move between areas in both directions at least 25 times.
- [ ] Test missing target, corrupt FPM, unexpected exit, full inventory/health state and restart mode `allowfragmentation` in a controlled copy.
- [ ] Measure time for first visit and revisits with both switches disabled, one enabled and both enabled; use the existing loadtrace profiler.
- [ ] Monitor system RAM, VRAM, handle count, thread count and asset validity for growth, stale files or missing textures.
- [ ] Verify a modified FPM invalidates cache and that scenes still import/play after standalone export.
- [ ] Confirm environment prefetch improves net latency rather than causing disk contention.
- [ ] Enable only the proven improvements by default in a later release, then update the README results.

**Keep `master` and the Steam GameGuru installation untouched until real tests pass.** The experimental engine source resides in draft PR #1.
