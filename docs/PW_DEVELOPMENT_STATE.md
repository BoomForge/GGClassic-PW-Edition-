# GameGuru Classic — Pixelwolf Edition: Development State

**Purpose:** This is the canonical *short handoff* for continuing development in a new ChatGPT conversation. It is updated after meaningful, verified changes. Read this before treating an older chat transcript, roadmap item, or PR body as current state.

**State checked:** 23 November 2026  
**Repository:** https://github.com/BoomForge/GGClassic-PW-Edition-  
**Active development branch:** `work/pw-community-integration-20261121`  
**Draft pull request:** https://github.com/BoomForge/GGClassic-PW-Edition-/pull/1  
**Verified branch head before this documentation commit:** `930691cf84bfeb10bcc594199c272561d1a963f9` (Asset Inspector preview fixes, browser filtering and dedicated documentation; inspect GitHub for newer commits).  
**Confirmed previous Windows x64 full compile:** [run #45012104941](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45012104941), **success**, for the _still broken in live editor_ disappearance-fix revision `0bd43d92`. This demonstrates compilation does not validate multi-grass visuals. The latest **Asset Inspector blank-preview and file-filter correction** has [full Windows x64 build #45158073815](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073815) still **in progress** at `73eee8d2`; its [UI regression guards #45158073857](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073857) **passed**, and [Windows import/cleanup validation #45158073936](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073936) **passed**. Newer source/doc commits trigger additional builds; check current HEAD result before installing. Updater is already integrated into the owner's editor. **Recheck Actions before claiming the latest full build passed.** The separate Windows updater EXE is already part of the owner's build; it also [compiled successfully at `9248ed24`](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44999138922).

> **OWNER-CORRECTED DELIVERY FACT (22 Nov 2026): The current installed PW Edition ALREADY INCLUDES AN IN-ENGINE UPDATER.** Never describe this updater as a future feature, or default to downloading source ZIPs and running `START_PW_TEST_BUILD.cmd` every update. The editor's update command launches the installed `Pixelwolf-GGC-Updater.exe` source-building helper. Distinguish this **existing, owner-confirmed capability** from whether the **latest revision** has passed actual installation and gameplay acceptance; CI compilation is not a local runtime test.

## Product intent and hard requirements

- Creator-friendly, fast, familiar GameGuru Classic world-building workflow, with early-2000s game-making simplicity, retaining the Classic visual style and icons rather than replacing it wholesale.
- Engine for **Adventuring for Dummies**: initially just a **very small Merravale exterior town**; preserve the broader fantasy/dungeon-crawling story but defer RPG systems, complex NPC work and quests until the town-building tools are usable.
- Editor layout: scene hierarchy on left, viewport in centre, Inspector/properties on right, **permanently docked** asset browser along bottom. Do not create an unwanted duplicate floating browser.
- Hierarchy supports groups/nested movement; Lua Studio is available; maintain responsive asset placement, original toolbar icons, and editor UI linkage. Owner previously marked these issues completed, but new revisions still require regression checks.
- **FBX is the supported importer to test. GLB import was explicitly disabled/deferred** after failed use; never promise current native GLB support. A Model Bridge frontend having GLB preview code is not equivalent to GameGuru GLB import.
- **Custom grass/skybox import is texture-only.** The user chooses a single image. The engine clones an existing native grass or skybox template and applies the image automatically. It is **not** an FBX/model importer and should not present mesh, scale, yaw or manual setup options. Support several grass types with a weighted spread/mix control.
- Support both PNG and DDS without breaking Classic DDS compatibility.
- Keep `master` protected; implementation is in the active development branch and draft PR until tested. Do not distribute proprietary upstream GameGuru media or binaries without checking rights.

## New urgent editor regression — Asset Inspector blank 3D scene and noisy listing (23 November 2026)

**Owner's live editor report:** The Asset Inspector opens but its right side displays a blank scene instead of a 3D model, and its left tree contains thumbnails and setup files. User explicitly requires a visually rendered rotatable 3D model, actual texture images, and **only viewable files** in the tree. This issue is independent of the still-unverified mixed-grass renderer.

**Source changes staged in development branch through `73eee8d2`, detailed handoff in [PW_ASSET_INSPECTOR.md](PW_ASSET_INSPECTOR.md):**
- `PW-AssetInspector.h`: filter the tree to `.dbo/.x/.fbx` models and `.png/.dds/.jpg/.jpeg/.bmp/.tga` images. Do not show `.fpe`, source metadata, thumbnails, thumbnail/preview folders or empty directories. Cache a bounded directory index, with **Refresh Files** for newly added assets.
- Keep `.fpe` metadata **internal only** to identify the original model albedo texture; match sibling model paths to `model=` and resolve `textured=` without displaying setup files.
- Inspector previously allocated preview models inside GameGuru's realtime object range (26400) and images within live editor image range (71750). Preview now probes isolated free IDs beginning at **220000 for objects** and **120500 for images**.
- Load a dedicated basic entity shader for the actual raw DBO/X/FBX preview object; initialise standard `EntityEffectControl` and restore its texture after effect assignment. Release shader/model/images on close/change.
- `M-GridEdit.cpp`: in Inspector-only offscreen thumbnail capture, stage the object high above terrain, use its actual bounding size/centre for camera position/look-at and zoom, skip live terrain and cascaded shadow passes. Native tooltip path remains unchanged.
- Keep drag rotation, mouse wheel zoom, Reset View and Insert into Scene; add **Refresh 3D Preview** and a capture-error message if the capture image is missing.
- `tools/test_pw_editor_ui_regression.py`: static regression checks for viewable file filter, thumbnail exclusion, internal sidecar matching, model effect, safe IDs, isolated framing, preview retry and original entity insertion.

**Checks:** UI regression suite [#45158073857](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073857) **passed** at source `73eee8d2`; import/cleanup suite [#45158073936](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073936) **passed**. Full x64 compile [#45158073815](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073815) **in progress** at last check. **NO actual local 3D picture has been verified.** The source code is a repair attempt, not visual acceptance.

**Next owner acceptance:** After full Windows CI success, use the **existing built-in PW Edition updater**, check installed commit and open Asset Inspector. Select a stock DBO/X model and an FBX; verify model (not just background) appears, rotates, zooms and textures display. Ensure FPE/setup files, previews/thumbnails and empty folders do not appear, and original bottom docked browser is unchanged. Use Insert into Scene and Undo and save/reopen. If still blank, capture screenshot and any preview error; do not report fixed solely because CI compiled.
## Current highest-priority task — actually render 2+ grass types (22 November 2026)

**Owner's fourth live observation:** Grass now renders after the safety gate, but only **one species** is visible when multiple grass options are selected. The safety gate **intentionally** left the original Classic single-species renderer running and set `Preview experimental multi-grass` **OFF** by default. That prevented all grass disappearing, but did **not** deliver multi-species rendering. Do not call this fixed merely because the native grass is back. Earlier disappearance and flicker are documented in [PW_WORKSPACE_SKY_GRASS_IMPORT.md](PW_WORKSPACE_SKY_GRASS_IMPORT.md).

**New source-level attempt (feature code through `a8b68916`):**
- Investigated the real render path rather than repeating mask/weight tweaks. Unlike imported GameGuru entities and the native vegetation grid, the separate PW grass source objects loaded a model and texture **without assigning a graphics effect**. Objects can therefore be allocated and marked visible while not rendering expected pixels in the DX11 editor.
- `M-Grass.cpp`: assign each **secondary grass model** its own `loadinternaleffectunique` standard entity shader (basic/PBR based on current render mode); use `SetObjectEffect`, then explicitly restore its own grass atlas with `TextureObject` because assigning a shader can reset texture stages. Configure `EntityEffectControl` to GameGuru importer standard, and clean up owned effects when deleting grass instances. Keep individual style meshes, images, scale and yaw. Do not duplicate the selected native grass type on top of itself.
- Make the preview an **overlay** rather than a replacement. `M-Grass.cpp`, `M-Terrain.cpp` and `M-GridEdit.cpp` now preserve the native Classic grid during startup, painting refresh, grass loop and post-shadow restore. Preview adds other enabled grass styles as shader-backed objects; it cannot intentionally blank native grass. Shadow rendering still hides and restores the supplementary instances appropriately.
- Preview diagnostics now show *per-style* object counts in addition to loaded models, placed objects and visible flags. Updated UI regression guards assert native grid preservation, separate shaders and texture binding order. [Targeted UI checks #45014969086](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45014969086) **passed** at `a8b68916`; import tests and full x64 build run [#45014968979](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45014968979) were in progress at last check.
- **Critical limitation:** This is an experimental additive overlay, not yet a fully weighted final native-grass integration. The existing full-density Classic base and at most 320 extra objects mean the on-screen 50/50 etc. *selection weights are not yet proven to match actual rendered visual proportions*.

**Next local acceptance, after full Windows x64 CI passes:**
1. Use the **existing PW Edition in-editor updater**, verify installed commit; do not default to downloaded branch ZIPs or manual builds.
2. In a **disposable test map**, paint grass, choose two very different built-in styles and nonzero weights; enable `Preview experimental multi-grass`. Classic should remain visible and the second grass species should render as additive objects.
3. Capture both a screenshot of the viewport and **per-style instance diagnostics**. Visible object flags only establish API state, not graphics output.
4. If secondary objects report nonzero count but are invisible, inspect real shader/mesh/material/camera rendering. If no secondary objects are counted, inspect missing style assets, vegetation mask or preparation errors. If both species appear, then separately address density/weight accuracy, wind/shadow, save/reopen and test game before marking multi-grass complete.
5. Until visual acceptance, preview defaults OFF and single Classic grass is a deliberately safe fallback. **Do not merge to master or claim true multi-grass works.**

**Detail:** [PW_WORKSPACE_SKY_GRASS_IMPORT.md](PW_WORKSPACE_SKY_GRASS_IMPORT.md).

## Previously added feature: Asset Inspector (source implemented, full CI compile passed; runtime unverified)

Specification: [PW_ASSET_INSPECTOR.md](PW_ASSET_INSPECTOR.md).

Owner request:
1. Separate **Asset Inspector** popup accessible from the docked Asset Browser.
2. Actual installed `Files` folder/directory tree at left.
3. Right pane previews actual image data (PNG/DDS/JPG/BMP/TGA) or real FPE/DBO/X/FBX model; rotate by mouse drag, zoom by wheel.
4. **Insert into Scene** creates a normal editable GameGuru entity in front of the editor camera, at terrain height, using native undo/placement flow.
5. Imported raw models use a separate user-controlled FPE/DBO copy; do not overwrite the original. Preview resources should be cleaned up.

**Source status:** Implemented and targeted UI regression checks passed. Initial compilation blockers in `PW-GrassMix.h`, `M-Grass.cpp`, and `M-MapFile.cpp` were fixed; updated updater diagnostic handling now aims to show actual MSBuild errors instead of trailing conversion warnings. Full x64 build containing the Inspector passed in run #44994266761 at `4716fe95`.

**Not verified:** That the popup opens on the owner's Windows editor; 3D geometry/texture fidelity; mouse orbit/zoom; insertion location; undo; save/reopen; raw FBX material copying/relinking; local installer. Do not turn a successful CI build into a runtime acceptance claim.

## Existing in-engine updater — REQUIRED continuity (owner correction, 22 November 2026)

**User-confirmed installed capability:** The owner's **current PW Edition build already has an updater inside the editor**. It has been the normal route for retrieving, compiling, and installing incremental PW Edition updates. Do not require a fresh ZIP/manual local build just because a new conversation starts.

**Code-backed workflow and distinction:**
- The editor exposes a PW Edition update command (PW Edition / Help update UI) that launches the installed **`Pixelwolf-GGC-Updater.exe`**. This source-building Windows x64 bootstrapper is already integrated into the current PW installation; it is **not** the older `pw-updater.ps1` release-only helper.
- [`tools/PWBootstrapper/Program.cs`](../tools/PWBootstrapper/Program.cs) checks the GitHub development branch, pins a source commit, downloads the source, compiles the GameGuru editor **locally using the owner's licensed Steam media and VS2022/Build Tools**, verifies that the new EXE is PW-branded, requests confirmation, backs up installed files, validates hashes, and offers to launch the editor. The updater itself can be refreshed from the **updater-only** GitHub release channel. No publicly downloadable proprietary GameGuru editor binary is required.
- The legacy `pw-updater.ps1` implements **stable versioned engine release ZIP** updates and is a *separate, currently secondary path*. The lack of a stable engine ZIP does not mean the built-in source-building updater is missing or unusable.
- **Default for future changes:** commit source changes; verify the relevant GitHub Actions builds; use the **already installed in-editor updater** to compile/install the next revision with its prompts and backups; then confirm the installed source commit and editor behaviour. **Do not tell the owner to download a ZIP or run `START_PW_TEST_BUILD.cmd` for each ordinary update.**
- **Fallback / safety:** the independent `START_PW_TEST_BUILD.cmd` / `tools/pw-local-test.ps1` route creates a separate test installation for diagnosing regressions or a broken updater. The built-in updater can replace files in the selected local installation after approval, so backups, version verification and actual launch testing remain important. A previously reported updater failure was a real compile error; newer source fixed the identified C++ errors and improved diagnostics, but subsequent errors must be examined from the full updater log.
- References: [current source-building updater](PW_WINDOWS_UPDATER_EXE.md), [separate legacy stable-release helper](PW_EDITION_UPDATER.md). **The in-engine updater's presence is owner-confirmed**, while the exact newest revision's local installation remains to be checked.

## Current delivery and build truth

| Stage | Verified state |
| --- | --- |
| Code committed on active branch | Mixed-grass shader overlay through `a8b68916` (visual unverified); Asset Inspector render/file-browser source through `73eee8d2`, docs through `930691cf` before this handoff update |
| Targeted source and UI checks | Passed for the Asset Inspector revision |
| Entire GameGuru Core Release x64 CI compile | Previous revision passed but failed grass visual check; latest Inspector source #45158073815 **in progress** at last check; independently check HEAD build |
| Compiled editor included as downloadable artifact in that CI run | **No** — artifact list empty |
| Local Windows editor test / original Steam overlay | **Not verified** for this revision |
| In-engine updater included in owner's current build | **Yes — owner-confirmed**; integrated source-building updater is already present |
| Latest revision installed and accepted using in-engine updater | **Not independently verified**; requires local update result, installed commit and runtime test |
| Production-ready release / PR merged to master | **No** — PR #1 is draft |

CI compilation resolves the previous reported compiler failure; the **built-in source-building updater already exists** and does not require a publicly distributed GameGuru engine artifact. Passing CI does not guarantee the newest local update or runtime will succeed.

## Local Windows test-build preparation (22 November 2026)

**Source-only implementation and checks (not an owner-tested build):**
- Updated [`tools/pw-local-test.ps1`](../tools/pw-local-test.ps1) to validate that Steam source contains Files and GameGuru.exe; reject unsafe destinations (Steam, repository root/unrelated subfolders, build output, test-directory reparse point); check required PW files and the compiled editor branding before staging; record the source commit and editor SHA-256; verify the staged editor hash. `-SkipBuild` requires a matching source-commit marker rather than silently reusing an unrelated binary.
- Updated [local build instructions](PW_LOCAL_WINDOWS_TEST_BUILD.md) and the generated `PW-TESTING-README.txt` to prioritise Asset Inspector, FBX thumbnail/placement, sky/grass texture-only cloning, scene save/undo/docking and explicit GLB deferral.
- Existing root [`START_PW_TEST_BUILD.cmd`](../START_PW_TEST_BUILD.cmd) remains the one-click launcher: clone/download the current branch, install local VS2022 C++/MFC prerequisites, double-click and select your licensed Steam Classic folder. Script compiles on the **owner's Windows PC** and stages an isolated `PW-TestInstall` copy. No licensed engine binaries/Steam media are published as CI artifacts.
- [Windows local-script CI](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/workflows/pw-local-build-script-validation.yml) checks PowerShell syntax and rejects harmful destinations using synthetic placeholder Steam files. The first negative-path test harness left an expected non-zero child exit code (run #44999006585); the correction at `e0dcd44e` **passed** Windows PowerShell syntax, destructive-destination rejection and Steam import-metadata checks in [CI run #44999063741](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44999063741). This is not a local editor or Steam acceptance test.
- This is **not** evidence of a successful local Steam overlay, editor launch or visual preview. It is a safer path to obtain those results without distributing the proprietary executable.

### Fallback isolated-test instructions (NOT the default update method)

Use `START_PW_TEST_BUILD.cmd` and the separate `PW-TestInstall\\Guru-MapEditor.exe` **only for safe isolation, or when the integrated updater is failing**. For ordinary ongoing revisions, the **existing in-editor PW Edition updater** remains the expected first route, with approval prompts and backups. The generated `PW-TESTING-README.txt` documents the manual test route.

## Next actions, in priority order

### P0 — Verify Asset Inspector rendering; separately finish multi-grass

- [x] Commit Asset Inspector source repair: only viewable models/textures in tree; no setup files or thumbnails; independent preview shader, resource IDs, model framing, texture sidecars and retry.
- [ ] Confirm newest full Windows x64 build for Inspector corrections, then **visually test an actual model** in the local editor (not just static tests). Use the already-integrated updater.
- [x] **Record third grass failure:** earlier overlap/instance/mask fixes compiled but did **not** fix the actual all-grass-disappears bug.
- [x] Add a **safe default**: selecting multiple grass types no longer automatically hides Classic; experimental PW rendering requires explicit preview opt-in. Add diagnostic resource/instance/visible-flag information in the editor.
- [x] Trace missing standard object shader on secondary grass source objects; add unique entity effect, set importer control constant, rebind per-style atlas *after* shader; overlay secondary species while retaining native grid; skip duplication of native species.
- [ ] **True simultaneous multi-grass remains visually UNVERIFIED.** Await preview screenshot showing two distinct species, along with per-style object count. Then tune genuine visible weight distribution, wind, shadow and game export.
- [ ] Confirm full Windows x64 compile of **shader-backed secondary overlay** and latest source/UI checks; check latest Actions status.
- [ ] With existing in-editor updater, install the **new overlay revision**; on a disposable map select two styles and tick **Preview experimental multi-grass**. Confirm Classic remains visible AND the second type is visible; capture per-style diagnostics and viewport. Preview OFF remains native-only safety mode, not multi-grass acceptance.
- [ ] If a *single* imported grass style still flickers with mix disabled, examine actual texture transparency and native UV atlas compatibility before changing shader/material processing.
- [x] Earlier Asset Inspector-inclusive full Windows x64 compile succeeded in run #44994266761.
- [x] Implement/source-check the **safer local build/staging path** and acceptance checklist; Windows local-script validation passed in run #44999063741. No compiled proprietary artifact has been published.
- [x] **In-engine updater already exists in the owner's current installation** (owner-confirmed and source-backed). Do not rebuild or manually ZIP-install the editor as the routine update process.
- [ ] **Next local acceptance:** use the existing PW Edition in-editor updater to build/install the new revision with its backup/confirmation flow, launch PW Edition and verify the installed commit. If the updater errors, inspect its logs before falling back to the separate local test-build route.
- [ ] Run **local editor smoke test**: confirm toolbar and permanent docked bottom browser; Asset Inspector tree filters out FPE/setup/thumbnails/empty folders; actual DBO/X/FBX 3D preview appears, rotates and zooms; PNG/DDS images display; Refresh Files and Refresh 3D Preview work; insert native model into scene, verify camera-relative placement, undo, hierarchy, map save/reopen.
- [ ] Confirm FBX thumbnail generation, texture-path handling and scene placement; verify no failed imports create ghost assets.
- [ ] Confirm cloned skybox and grass textures with multiple grass styles survive save/reopen and exported game. Confirm existing native mesh/UV behaviour.
- [ ] Independently validate the **existing updater's reliability** on a disposable PW install, including integrity checks, backups/rollback and optional restart. This does not mean the updater is missing from the current editor.

### P1 — Remaining editor workflow fixes

- [ ] Safely delete user-imported assets from Asset Browser (with exact-file confirmation, reference/dependency checks and Recycle Bin or recoverable quarantine). Never delete stock, DLC or shared asset files. See [roadmap](PIXELWOLF_CLASSIC_ROADMAP.md#editor-ux-request--safely-remove-custom-assets-22-november-2026).
- [ ] Revisit Model Importer 2 after basic FBX preview and insertion stability: materials, proper UVs, transactional export, rig playback, Mixamo compatibility, skeletal attachments and eventual Character Creator improvements.
- [ ] Preserve existing source-level improvements to the Lua editor, grouped hierarchy, scene layout, thumbnail browsing and performance. Do not call previous user acceptance a substitute for regression testing.

### P2 — Later game/export features (deferred for now)

- [ ] Simple windowed/fullscreen and resolution options, with a safe fallback.
- [ ] Player-profile game save/load scheme, with resilient persistent variables.
- [ ] Projectile collision/damage regression; custom control remapping.
- [ ] Level-transition stability, repeat-visit speed and resource-leak testing across multiple levels.
- [ ] GLB import only after a separately approved, working implementation; currently deferred.
- [ ] Broader RPG systems only after Merravale town-building workflow is solid.

See [README.md](../README.md), [technical roadmap](PIXELWOLF_CLASSIC_ROADMAP.md), [Asset Inspector acceptance](PW_ASSET_INSPECTOR.md), [texture-only sky/grass specification](PW_WORKSPACE_SKY_GRASS_IMPORT.md), and [updater notes](PW_EDITION_UPDATER.md) for the detailed backlog and feature-specific constraints.

## Protocol for every future development chat

1. Read **this file first** on the **active development branch** and check PR #1 plus the branch's actual latest commit. If another branch/PR supersedes it, update these fields rather than pretending this source SHA is still HEAD.
2. Check the latest relevant Actions runs, statuses, artifacts and logs. Distinguish a passed compile from a testable install and user acceptance.
3. Read feature-specific docs/source for the requested changes. Prefer actual repository state over verbal claims from an earlier assistant.
4. Do the requested engineering work on a development branch, run available checks, and report specific commits, CI results and any untested limitations.
5. **Before ending a development turn with meaningful changes, update this handoff** with the latest verified state, blockers and next three concrete actions. If changes require a roadmap update, update its specific line as well; do not duplicate every long changelog here.
6. **Read the handoff back from GitHub after writing**, to verify persistence. Avoid claiming writes or builds succeeded until tool results confirm them.
7. Do not reintroduce owner-rejected behaviour: undocked duplicate browser, altered Classic icons, GLB presented as working, or mesh-based custom grass/skyboxes. **Never forget the already-integrated updater, or default to manual ZIP installation.**
8. When owner requests "continue GG Classic" in a fresh chat, use the repository first and proceed from the next uncompleted gate; ask only for genuinely missing inputs that block actual work.

### Suggested resume message

> Continue GameGuru Classic — Pixelwolf Edition from `docs/PW_DEVELOPMENT_STATE.md` on the active GitHub development branch. Check latest commits and CI before making changes; update and read back that file after completing work.
