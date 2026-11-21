# The GameGuru Classic Repository

This is not open source and remains the copyright of The Game Creators Ltd, see Wiki for more information on license matters. This repository contains a 64-bit version of Classic as the master trunk, and an older version under the 'VS2017-Legacy-JAN2023' branch that used the older VS2017 for compiling.

GameGuru Classic is a 3D game maker for simple first and third person games and is available to buy from Steam. 

Steam Product: https://store.steampowered.com/app/266310/?utm_source=githubrepo&utm_campaign=general&utm_medium=web

You will need the media files associated with the above product to use this repository.

This source code requires Visual Studio 2022 Community Edition. You can get this for free from the Microsoft website.

When installing Visual Studio 2022, ensure you select the "Desktop C++" development track, and tick the optional Visual C++ MFC for x86 and x64 as you will need these additional files to compile the legacy GameGuru Classic executables.

To give you a better idea about getting started, a while ago we made a 12 minute video showing the whole setup process from the very first download to running GameGuru Classic under your own binary creations. This video shows VS2017 being used but the process remains the same.

How To Use Repository Video: [[Click To Watch Video]](https://www.youtube.com/watch?v=gtkmwEDivxo)

## F.A.Q

Q> Is GameGuru Classic open source?
A> No, the product and it's source code is copyright The Game Creators Ltd. We make the source code available so contributors from the community can integrate additional functionality, and to enable any user to scan through the code to find out how the inner workings of this game maker works.

Q> I can no longer find the LFS large media files in the Repository?
A> Due to this public repository being hammered by anonymous cloning, we have moved the media files to "GAMEGURUREPO_LARGEFILES.zip", available from our GDrive at: https://drive.google.com/file/d/17nCbOLPEbfHLbgN8ua7CgJQJMWpELQGG/view?usp=sharing

---

## Pixelwolf Edition — Feature Upgrade Tracker

**Project:** [GGClassic PW Edition](https://github.com/BoomForge/GGClassic-PW-Edition-)  
**Development branch:** [work/pw-community-integration-20261121](https://github.com/BoomForge/GGClassic-PW-Edition-/tree/work/pw-community-integration-20261121)  
**Review:** [Draft PR #1](https://github.com/BoomForge/GGClassic-PW-Edition-/pull/1)  
**Last updated:** 22 November 2026

The goal is a **fast, familiar first-person world builder** for an early-2000s-style 3D RPG that will grow gradually over a long period. DDS support must remain intact. New graphics formats are **additional**, not replacements.

This tracker reports the latest **development-branch** progress. The default `master` branch may not contain those source changes until the draft PR is tested and merged.

Statuses below distinguish **committed source changes** from **tested runtime behaviour**. A checked source task does not mean a Windows executable has been built or verified.

### Current milestone — Model Importer 2 M1 (22 November 2026)

- [x] **User-confirmed editor acceptance:** The owner marked the corrected updater, original toolbar icons, Classic style, docked bottom asset browser, asset drag delay, nested hierarchy and Lua Studio as completed. Treat these as the owner's sign-off; they are not a new independent benchmark of every machine.
- [x] **Model Importer 2 M1 preflight:** A new read-only ImGui panel inspects mesh/vertex/skin counts, frame parents, referenced texture stages and source animation clips. It checks source and FBX-converter folders for existing DDS/PNG/JPG/TGA material paths, warns about unresolved references and malformed tables, and can copy a versioned `pw.asset-preflight` JSON report.
- [x] **Reusable manifest and isolated Windows checks:** `PW-AssetManifest.h` is a shared contract for the later Character Creator 2 workflow; `test_pw_asset_manifest.cpp` exercises material fallback, hierarchy, clips, JSON escaping and damaged-object handling. It is wired into the Windows skeletal-import CI.
- [ ] **Importer 2 remaining:** Full UV/axis inspection, material relinking UI, safe transactional DBO/FPE export, real rigged FBX playback, Mixamo retargeting, and complete Windows editor/gameplay acceptance. **The legacy importer is intentionally unchanged.**

### Completed — checked and committed in source

- [x] Forked the latest official **64-bit GameGuru Classic** source. The initial fork matched upstream commit `821f688` exactly.
- [x] Audited community forks: the investigated Preben/plemsoft and Cybernescence performance improvements were **already included upstream**, so no older engine changes were blindly cherry-picked.
- [x] Staged the community animated alpha/emissive PBR shader (upstream [PR #6261](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/pull/6261)); **runtime test outstanding**.
- [x] Added a **DDS-first PNG fallback** to entity texture lookup in `M-Entity.cpp`, including diffuse/base, normal, specular/metalness, gloss, AO, height, illumination/emissive/detail, and supported multi-material override paths. Existing explicit `.png` paths continue to load normally; **runtime test outstanding**.
- [x] Extended `M-Importer.cpp` to find same-name `.png` files when a converted FBX's referenced texture cannot be located, while continuing to prefer an available `.dds`.
- [x] Extended `M-MapFile.cpp` to preserve PNG `_D`, `_N`, `_S` companion files in the existing level/map archive workflow without removing DDS handling.
- [x] Created a detailed [technical roadmap](docs/PIXELWOLF_CLASSIC_ROADMAP.md) of remaining engineering risks, source references and test criteria.

- [x] Wired **Help > Check for PW Edition Updates...** into the active 64-bit ImGui editor source, using a local update helper rather than the disabled legacy updater. **Windows UI test outstanding**.
- [x] Committed a GitHub Releases updater with version comparison, HTTPS download, SHA-256 verification, allowlisted package contents, staged installation after the editor closes, file backups/rollback, optional Windows UAC, and **restart confirmation**. **Source committed, not yet tested on Windows**.
- [x] Updated the 64-bit build project to copy the updater and PW Edition version marker next to the editor executable.
- [x] Added an explicit-release ZIP and SHA-256 packager, packaging contract tests, and [updater setup/release instructions](docs/PW_EDITION_UPDATER.md). [Windows CI run #44816392164](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44816392164) **passed package tests and PowerShell syntax checks**. No engine binary or update release has been published yet.

- [x] **Instrumented Test Level loading** in the 64-bit editor and game source: test-map save, environment refresh, terrain sampling/heightmap creation, entity-bank and terrain loading, AI, scripts and media. Profiling is **opt-in** with `GGCPW_PROFILE_LOAD=1` and creates per-process CSV logs; standard launches don't log. This is diagnostic code, **not a confirmed speedup**.
- [x] Added [load profiling instructions](docs/PW_LOADING_PROFILE.md), a Python CSV ranking tool and a Windows smoke test. [GitHub Actions run #44817890932](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44817890932) **passed** the Python report tests, isolated Visual Studio x64 compilation, and a real opt-in CSV-writing smoke test. Full engine build remains unverified.

- [x] **Safer standalone level jumps (source):** scripted transitions now check the target FPM before ending the current level. Missing or unsafe destinations are rejected without destroying the active level, and repeated mapbank subfolder lookup can reuse a verified path.
- [x] **Optional repeat-level extraction reuse (source):** added an FPM extraction cache keyed to the unchanged archive's size and modified timestamp, with cache-file verification and fallback to Classic's original extraction method. **Off by default** via `GGCPW_FPM_CACHE=1` until the engine passes real-world tests.
- [x] **Connected-area prefetch foundation (source):** during the existing level transition fade, a bounded low-priority worker can pre-read the next FPM to warm Windows' file cache. **Off by default** via `GGCPW_PREFETCH_LEVELS=1`; it does not promise seamless loading. See the [transition implementation and testing guide](docs/PW_LEVEL_TRANSITIONS.md).
- [x] Added isolated Windows x64 source and runtime tests for level lookup, missing targets, corrupted-cache rejection, root-vs-nested map precedence, cache invalidation and optional prefetch. [CI run #44821610434](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44821610434) **passed Visual Studio C++ compilation and synthetic runtime tests**. These do not compile the entire GameGuru engine.

- [x] **Cross-level health and weapon continuity (source):** added an opt-in `GGCPW_KEEP_LEVEL_STATS=1` path for ordinary standalone in-process `JumpToLevel` transitions. It reuses Classic's existing health, lives, 10 weapon slots and ammunition save/load routines, with an in-memory health/lives fallback if a fresh Lua file is unavailable. Original process-restart handling is retained.
- [x] **RPG quest/item-state foundation (source):** new `PWSetTransitionValue` / `PWGetTransitionValue` / `PWClearTransitionValues` Lua APIs persist explicitly managed simple flags, counters and strings across connected levels in a non-executable validated data file. This is **not** an automatic RPG inventory system.
- [x] **Defensive save parsing and delivery (source):** the original v1 weapon-stat loader now validates all 43 values before applying any; standalone export, x64 build copying and the release packager recognise the new `scriptbank/pw_transition_state.lua` module. See [persistent level-state design and deployment](docs/PW_PERSISTENT_LEVEL_STATE.md). **No complete engine build or game test has been performed.**

- [x] **Safer per-level cleanup (source):** replaced the original never-ending occluder worker loop with cooperative stop/join, then release its Win32 worker and event handles before entities, AI and physics are unloaded. Added a profiler phase for shutdown. See [cleanup and importer engineering notes](docs/PW_IMPORT_AND_LEVEL_CLEANUP.md). **Full-engine verification pending**.
- [x] **Blender/Mixamo FBX import hardening (source):** safe, case-insensitive FBX detection; find missing textures by basename alongside the model; retain DDS with additional PNG/TGA/JPEG fallback; guard null meshes, short filenames, full texture lists and legacy fixed-buffer path handling. This **does not yet fix bone/clip retargeting**.
- [x] Added isolated VS2022 Windows x64 importer path and repeated worker-start/stop checks. [CI run #44838327649](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44838327649) passed those synthetic tests. The isolated checks pass at the latest development source commit; full GameGuru compilation and actual FBX/level behaviour remain unverified.

### Windows build and local Steam overlay preparation (21 November 2026)

- [x] Removed the machine-specific D-drive editor output paths, staging PW binaries in the repository-relative PW-Build/Release directory.
- [x] Staged the new optional shader alongside the updater and Lua companion files.
- [x] Added an opt-in, source-only Windows x64 full-solution compile probe. Until this passes, the fork is **not** a proven working binary.
- [x] Added local overlay packaging, checksum verification, preview-first installation with backups, and a Windows compile-and-package command. The new checks are awaiting CI execution.
- [ ] Verify a complete build and overlay against a licensed Steam install; no public binary is distributed.

### PW Lua Studio (source implemented; Windows validation pending)

- [x] Added Edit > Lua Script Editor... as a native modeless window inside Guru-MapEditor.exe (Windows RichEdit, not an external IDE). Open/New/Save/Save As and dirty-change confirmation included.
- [x] Added syntax colouring for Lua keywords, comments, strings, numbers and known GameGuru functions, a searchable built-in command browser based on installed global.lua and PW state functions, and Ctrl+Space prefix completion.
- [x] Scripts save as UTF-8 with a timestamped backup of the prior file; editing never executes or hot-reloads gameplay code.
- [ ] Confirm full VS2022 compilation, editor focus, file I/O and coloured text on Windows. The editor is a separate native tool window (not yet ImGui dockable); autocomplete is symbol-based, not a language-server engine. See [Lua Studio notes](docs/PW_LUA_STUDIO.md).

### In progress — verification needed

- [ ] **Level cleanup regression:** in a compiled editor and standalone game, inspect thread and event-handle counts over 25+ transitions, plus stalled occlusion and return-to-editor cases. The isolated C++ thread test is not a full game test.
- [ ] **Blender/Mixamo importer regression:** verify static/rigged FBX with valid/missing/renamed PNG and DDS textures, multi-material meshes, bones and animation clips. C++ source is committed, but actual animation correctness and full-engine compile are unverified.
- [ ] **Complete transition regression testing:** build the entire 64-bit engine and a two-/three-level standalone; switch repeatedly (25+ round trips), observe failures, RAM/VRAM/handle growth and player state retention.
- [ ] **Benchmark both optional transition optimisations:** compare cold first loads and repeat visits with/without `GGCPW_FPM_CACHE` and `GGCPW_PREFETCH_LEVELS`. Enable either by default only if the result is both faster and stable.
- [ ] **Persistent connected-area gameplay state:** health, lives, weapon slots/ammo and explicitly marked Lua quest values now have source-level continuity hooks, but these remain untested. Verify across real levels and restarts; checkpoint persistence, complete custom inventory integration and early entity-script access are still future work.

- [ ] **Updater end-to-end acceptance:** build and install a disposable PW Edition copy, publish a verified release ZIP/checksum, trigger the in-engine Help menu updater, test download, safe installation, rollback/error handling and the restart prompt. No Windows build or live update has been verified.
- [ ] **Update-release signing and provenance:** verify the distribution licence, strengthen publisher authentication, and test UAC in a protected folder.
- [ ] **PNG + DDS regression test on Windows:** build and load equivalent sample entities using `_D.dds` and `_D.png`; confirm diffuse, normal, metalness/specular, AO, gloss, height, emissive, transparency, material overrides, save/reopen, map archives and standalone exports.
- [ ] **Shader validation:** compile and run the community animated alpha/emissive PBR shader; compare unchanged default shaders for regressions.
- [ ] **Build and stability baseline:** compile the fork using Visual Studio 2022 x64 with the original legally owned GameGuru media and required dependencies; test editor launch, tiny FPS level, play/stop, save/reopen and standalone game.
- [ ] **Loading-time investigation:** profiling hooks are committed and individually Windows-tested, but the full PW Edition engine has not been benchmarked. Measure cold startup, first test, repeat test, return to editor, and standalone load for an empty world and a populated small town. Then optimise the measured bottleneck and verify before/after results.

### Model Importer 2 and Character Creator 2 — approved direction

- [x] Native skeletal import inspection UI, non-destructive Mixamo/Bip01 mapping preview, and opt-in DBO save/reload check. [Implementation notes](docs/PW_SKELETAL_IMPORT_AUDIT.md). Runtime animation retargeting and character-creator rebuilding remain outstanding.
- [x] Shared native deep skeletal DBO audit now checks bone bind matrices, per-vertex indices/weights, frame rest matrices and animation key payloads in both the importer and Character Creator exports, opt-in only. Standalone Windows integrity tests pass. Full game export/playback and retargeting remain unverified.
- [ ] Rebuild both tools around shared asset material, rig, animation and export contracts, preserving legacy GameGuru features during migration. [Rebuild and Model Bridge research](docs/PW_MODEL_IMPORTER_CHARACTER_CREATOR_REBUILD.md).
- [x] **Recover v1.0.7 embedded Model Bridge frontend sources from the owner-provided Go EXE:** 33 files, including FBX/GLTF/GLB preview, skinned DirectX X handling, mesh sampling and original tests; 30 JavaScript files passed syntax-only checks. Source packaged for owner; technical inventory in [rebuild research](docs/PW_MODEL_IMPORTER_CHARACTER_CREATOR_REBUILD.md). Not yet ported wholesale into the GameGuru native importer.
- [ ] **Rebuild missing Go backend functionality and implement GameGuru native skeletal retarget/export:** web source recovered, but HTTP backend remains compiled, MD3 final format loses skeletal data, and the older v1.0.4 source ZIP is still unavailable.
- [ ] Evaluate the separate M.I.L.O retro-character-creator-foundation branch for canonical low-poly body/rig, morph, hair/clothing and export design.

### Local Windows test builds

- [x] Safe local build-and-stage script for owned GameGuru Classic Steam files: [tools/pw-local-test.ps1](tools/pw-local-test.ps1). Compiles x64 in VS2022, stages into a separate `PW-TestInstall` folder, and does **not** overwrite Steam or publish proprietary EXEs.
- [x] CI now restores the pinned official Microsoft D3DX import libraries (`d3dx11.lib`, `d3dx9.lib`) locally and builds vendored DirectXTex/Effects11 dependencies before full engine linking. [Build instructions](docs/PW_LOCAL_WINDOWS_TEST_BUILD.md).
- [ ] Confirm full x64 CI engine + launcher build success and verify gameplay in a local Windows test install.

### To do — future upgrades

- [ ] Build an optional **full RPG inventory and quest save system** on top of the PW typed transition-value API; currently scripts explicitly select which simple values to carry.
- [ ] After testing basic level transitions, investigate **fully seamless/streamed interiors** as a separate, larger architectural project; current prefetching is not live-scene streaming.
- [ ] Improve the **in-engine update experience** with a native ImGui progress/status panel after the safe external installer is validated.
- [ ] Reduce unnecessary editor-to-test **loading times** after profiling; preserve caches and correctness. [Upstream #3100](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/3100).
- [ ] Complete **Blender/Mixamo skeletal animation** support (bone hierarchy/orientation, skinning, clip/frame ranges and retargeting). Texture lookup and crash safeguards are staged, not yet tested in the engine. [Upstream #1894](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/1894).
- [ ] Fix **texture overrides, material paths and missing textures** during import and game export. [Upstream #4659](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/4659).
- [ ] Research **glTF/GLB import** for static and rigged assets, without removing legacy X/FBX.
- [ ] Improve **live local-light shadows** for torches/lamps while retaining existing sunlight shadows and good GTX 1660-class performance. [Upstream #555](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/555).
- [ ] Investigate and improve **terrain performance and 64-bit object culling**. [Upstream #6223](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6223), [#5643](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/5643).
- [ ] Investigate **dynamic-light normal-map behaviour**. [Upstream #6209](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6209).
- [ ] Investigate **waypoint-editing stability** for later NPC work. [Upstream #6444](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6444).
- [ ] Confirm reliable Windows builds and regression checks before merging experimental engine changes to `master`.

**No Windows binary or performance improvement has been verified yet.** The first priority is maintaining a pleasant, fast world-building workflow—not building the entire RPG feature set now.

