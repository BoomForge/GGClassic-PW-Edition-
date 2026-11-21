# PW Edition — Test Level loading-time profiling

**Status (21 November 2026):** instrumentation committed in source; standalone Windows profiler compilation and runtime tests are run through GitHub Actions; **the actual modified GameGuru editor has not been compiled and benchmarked yet**. This work does **not** claim a speed improvement.

## Why this comes next

For *Adventuring for Dummies*, the immediate requirement is a fast FPS preview and small-world editing loop. Classic performs several independent tasks each time you select Test Level: save a test map, refresh sky/terrain/vegetation, sample terrain height, initialise entity assets, prepare game systems, create AI navigation, scripts and media.

In the existing editor source, `editor_previewmapormultiplayer_initcode` contains **1025 x 1025 = 1,050,625 calls** to `BT_GetGroundHeight` when terrain is present. This is a **candidate** bottleneck, not an established cause of the user's slow loads. Its results populate `t.terrainmatrix`; do not remove or skip it without verifying correctness and reproducible timing.

## Enable profiling

On a Windows computer with an installed **compiled PW Edition executable**:

1. Open Windows PowerShell in the editor installation directory.
2. Set `$env:GGCPW_PROFILE_LOAD="1"`.
3. Launch `./Guru-MapEditor.exe` from that same PowerShell session.
4. For each test: open a fresh empty flat level, press Test Level, return to editor, repeat. Then repeat on a small level with several imported Blender entities.
5. The profiler writes a CSV to `%TEMP%\GGCPW-loadtrace-<process-id>.csv`. It does not write files unless profiling is explicitly enabled.

After closing the editor:

    python tools/analyse_pw_loadtrace.py "C:\Users\<you>\AppData\Local\Temp\GGCPW-loadtrace-1234.csv"

The script ranks stages by median runtime; use `--top 50` to display more. The output is a **timing report, not an automated speedup**. Child scope durations are included inside parent scope durations, so do not sum all rows into a total.

To disable tracing, close the editor and restart it without the environment variable. Normal launches have almost no profiling overhead and create no profiling files.

## Instrumented source

- `GameGuru Core/GameGuru/Include/PW-LoadTrace.h`: Windows QueryPerformanceCounter RAII scopes, opt-in, per-process CSV log; writes only fixed stage labels and durations.
- `GameGuru Core/GameGuru/Source/M-GridEdit.cpp`: full preview preparation, saving the test map, refreshing environment references, terrain height samples and heightmap generation.
- `GameGuru Core/GameGuru/Source/M-Game.cpp`: entity bank and entity elements; waypoint and terrain loading; entity creation; gameplay preparation, HUD, weapons, AI path/character/obstacle setup, scripts and entity media.
- `tools/analyse_pw_loadtrace.py`: offline CSV report.
- `tools/test_pw_loadtrace_report.py`: report/statistics validation.
- `tools/test_pw_loadtrace_win.cpp`: isolated Windows compilation and logging smoke test.
- `.github/workflows/pw-loadtrace-validation.yml`: Python tests and Visual Studio 2022 x64 compiler smoke test of profiler header. **Does not compile the whole engine.**

## Benchmark acceptance before optimising core paths

- [ ] Compile full 64-bit engine in VS2022 with legally owned GameGuru media.
- [ ] Capture three cold starts and at least five repeated Test Level sessions for each of an empty flat scene and small populated scene.
- [ ] Save raw timings privately, or share anonymised CSV logs without project paths or proprietary assets.
- [ ] Rank and compare stages; identify actual wall-time bottleneck(s).
- [ ] Propose only targeted changes against proven bottlenecks; avoid unsafe caches or hidden global performance switches.
- [ ] Build again, compare before/after medians, check no broken terrain, collisions, entity imports, save/reopen, standalone export or shader changes.
- [ ] Update README with measured improvement only when repeatable. Until then, label as **profiling implemented; performance optimisation in progress**.

## Expected diagnostic questions

1. Is the terrain-height sampling stage slower than entity loading or environment setup in a small level?
2. Is the test-map save taking a disproportionate amount of time even without project changes?
3. Which stages improve on the second Test Level run, and which work repeats?
4. Are shader rebuilds being forced unnecessarily by setup settings?
5. How do a fresh empty level, a 10-object town scene and a larger scene compare?

**Do not optimise by removing the save-before-test safety behaviour.** An unexpectedly slow engine is frustrating; losing hours of game work is worse.

## Owner slow-project-load report and one-click capture — 21 November 2026

The owner reports that a test project eventually loaded after more than one minute, despite apparently repeating loading steps. This does not prove an infinite loop or identify a performance bottleneck.

In a repaired PW Edition build, enable PW Edition > Capture Load Diagnostics (CSV), reproduce the slow load, then open PW Edition > Open Load Diagnostics Folder. Look for GGCPW-loadtrace-<pid>.csv in the Windows Temp directory. The GGCPW_PROFILE_LOAD=1 environment-variable alternative remains supported.

Entries prefixed begin. identify a stage entered before an unfinished/hung operation; slow. entries flag completed stages lasting at least 10 seconds. The analyser ignores both marker types when ranking completed durations. If no useful trace appears, add instrumentation at the uncovered location rather than claim a speed-up.

Status: code change pending full owner runtime comparison; no measured improvement claimed.
