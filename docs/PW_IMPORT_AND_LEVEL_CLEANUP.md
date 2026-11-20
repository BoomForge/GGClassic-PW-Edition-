# PW Edition — level cleanup and Blender/Mixamo FBX import reliability

**State: source changes staged on the development branch, isolated Windows tests run; full 64-bit GameGuru build and gameplay tests deferred.**

This upgrade follows the earlier level-transition preflight, opt-in FPM cache, prefetch and cross-level state work. The objective is to reduce avoidable crashes and resource leaks **without redesigning Classic's core editor or breaking existing character assets**.

## 1. Close the previous level's occlusion worker correctly

### What the original Classic source did

At the start of `game_freelevel()`, Classic set `g_occluderOn = false`, woke its occlusion worker and waited for a finish event. However, `cOccluderThread::Run()` was a `while(1)` loop with no stop condition. It continued to hold its thread and synchronization resources between levels. The event wait protected an occlusion pass; it did not retire the worker.

### PW source change

- `Dark Basic Public Shared/Include/cOccluderThread.h` now checks the underlying `cThread` termination request, exits its loop after being woken, and signals the finish event on exit.
- `GameGuru/Source/M-Game.cpp` requests stop, wakes the worker, joins it, closes the Windows thread handle, deletes the occluder object and closes both begin/end event handles. Every pointer/handle is reset to null **before** the engine unloads the previous level's entities, AI, physics or textures.
- `game.transition.occluder_shutdown` is now a stage in the opt-in `GGCPW_PROFILE_LOAD=1` CSV profiler.
- The next level may create a **fresh** occluder worker; old level objects are no longer intentionally left in use by a still-running worker.

This fixes an identified lifecycle omission in source. It does not guarantee all memory leaks or rare deadlocks are gone: an occlusion calculation that never returns can still block a synchronous join. We deliberately avoid force-killing a thread that might still be reading engine objects.

## 2. Import external Blender/Mixamo textures more safely

### What was changed

- Added `GameGuru/Include/PW-ImportPaths.h` with safe, case-insensitive extension checks, basename extraction and a fallback search for textures next to a model.
- Converted FBX references may point to paths from the artist's computer, not the current installation. The importer now considers the original texture reference, same-folder basename, then extension companions. **Existing DDS is preserved**; PNG is additional. TGA and JPEG files remain supported.
- Removed several negative pointer-offset comparisons on filenames shorter than `.fbx`, `_color` or a supported texture extension. Unrecognised textures do not masquerade as JPG.
- Replaced a vulnerable `strcpy`/`strcat` fixed-size buffer in legacy texture scanning with safe string-based lookup.
- Added guards for empty/null mesh texture tables and empty filename references, and ensures the imported texture-working directory is restored on the error path.
- Avoids writing to texture-list slot zero when all importer texture slots are occupied.
- Sets the internal “FBX loaded” flag based on whether a model actually loaded, rather than assuming `LoadFBX` succeeded.
- Keeps the underlying FBX skeleton, bone weighting and animation tracks untouched. **This is reliability groundwork**, not a claim of fully corrected Mixamo animation import.

### Unsupported for now

- No new glTF/GLB import or runtime retargeting was added.
- No guarantee is made that imported Mixamo rigs are compatible with Classic's Uber/Bip01 animation system.
- Blender bone orientations, multiple animation clips, root-motion interpretation, and imported character retargeting require the eventual complete engine build and sample projects.
- The source code alone cannot establish how many textures or animations import correctly in real scenes.

## Source and isolated tests

- `tools/test_pw_import_paths_win.cpp` tests short filenames, case-insensitive FBX detection, absolute artist paths, DDS/PNG companions, missing textures and basename recovery.
- `tools/test_pw_occluder_thread_win.cpp` compiles the actual thread implementation against a synthetic occlusion callback and repeats start, signal, stop, join and handle cleanup across 12 cycles.
- `.github/workflows/pw-import-and-cleanup-validation.yml` compiles and runs both on Windows/Visual Studio 2022 x64 using synthetic inputs. The workflow does **not** compile the full editor or ship binaries.

## Deferred Windows acceptance

- [ ] Build the actual 64-bit engine with licensed GameGuru media/dependencies.
- [ ] Run Test Level > return > Test Level repeatedly; verify occluder worker count and handle count do not grow.
- [ ] Transition town ↔ interior ↔ dungeon 25+ times, tracking memory, render stability, audio and AI.
- [ ] Import Blender static mesh FBX with original DDS, PNG-only, mixed formats, multiple materials and absent texture paths.
- [ ] Import Mixamo skeletal FBX (with and without embedded textures), verify bones, clips, frame ranges, animation playback, skinning and export.
- [ ] Check no regressions to original legacy X/DBO model import, default asset bank, editor texture stage mapping or standalone export.
- [ ] Benchmark load times before/after; no numerical improvement is claimed until measured.

**Engine source remains unmerged in draft PR #1.** The public `master` branch contains the progress tracker and documentation, not these unverified engine changes.
