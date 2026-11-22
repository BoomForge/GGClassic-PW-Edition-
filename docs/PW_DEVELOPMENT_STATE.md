# GameGuru Classic — Pixelwolf Edition: Development State

**Purpose:** This is the canonical *short handoff* for continuing development in a new ChatGPT conversation. It is updated after meaningful, verified changes. Read this before treating an older chat transcript, roadmap item, or PR body as current state.

**State checked:** 22 November 2026  
**Repository:** https://github.com/BoomForge/GGClassic-PW-Edition-  
**Active development branch:** `work/pw-community-integration-20261121`  
**Draft pull request:** https://github.com/BoomForge/GGClassic-PW-Edition-/pull/1  
**Verified branch head before this documentation commit:** `0bdbfe762a6b156db044fd935d791aff98269753` (follow-up mixed-grass disappearance fix and test documentation; confirm newer HEAD before edits).  
**Last fully confirmed Windows x64 compile at the time of this handoff update:** [run #44999138941](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44999138941), **success**, for the previous editor revision `9248ed24`. The **first overlap fix** subsequently [compiled successfully in full Windows x64 CI #45009785335](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45009785335) at `e75bf86a`. The **subsequent disappearance fix** has [full Windows build #45011851690](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45011851690) queued/in progress at `ac7ab53c` during this read; its [UI regression guards #45011851792](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45011851792) **passed**. **Recheck Actions before claiming the latest full build passed.** The separate Windows updater EXE is already part of the owner's build; it also [compiled successfully at `9248ed24`](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44999138922).

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

## Current urgent regression — grass disappears when enabling multiple types (22 November 2026)

**Owner directly observed two revisions of grass problems:**
1. The early multi-grass implementation displayed only one species/current custom texture and flickered.
2. After the overlap/shadow fix, **selecting more than one grass type made ALL grass disappear**. This second regression takes priority; do not call the earlier update successful from CI compilation alone.

**Root causes traced in source, code changed on active branch (through `ac7ab53c`):**
- The original native Classic grid was previously drawn over mixed instances, causing overlapping textures and likely z-fighting. The earlier fix hid Classic whenever the mix was reported `Ready()`, but **`Ready()` meant only models/textures were loaded, not that a single mixed grass instance had actually been placed**. Thus a valid selected mix with an empty sampled area hid every blade.
- The mixer sampled only **21×21 terrain cells around the camera** and one pixel per **2×2 paint-mask** cell. Grass painted elsewhere in the editor or in small strokes could be missed. The mix now searches progressively outward up to 48 cells / 2400 world units, checks all four paint-mask pixels, favours nearby cells, and maintains the 320 object cap.
- Real `InstanceObject` copies mesh data, **not the source object's transform**. The mix previously scaled the *hidden source object* by the native 200%, leaving rendered instances at default size. Each rendered instance now explicitly receives its template's scale and yaw.
- The new `PWGrassMix::OwnsGrid()` is **true only when at least one actual mixed instance has been placed**. `grass_init` keeps Classic enabled initially, and editor startup, painting refresh, grass loop and post-shadow restoration only hide Classic if `OwnsGrid()` confirms real instances. Empty/invalid mixtures revert to the single-style Classic grid instead of blank terrain.
- Mixed objects are hidden/restored around cascaded shadow passes, avoiding both render paths drawing in the same pass; multiple chosen meshes retain separate texture atlas image IDs.
- Updated static UI checks cover ownership, fallback, mask sampling, 200% per-instance scale, shadow visibility and per-style textures. They **passed** in [#45011851792](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45011851792) at `ac7ab53c`. [Windows import and cleanup CI #45011835125](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45011835125) also passed on the earlier renderer commit `7d4c52c3`.

**Still unverified:** The **newest** full Windows x64 compilation, visible multi-grass output, saved/reopened project, no flicker, performance and standalone. Passing source tests does **not** mean the reported disappearing-grass fault is accepted as fixed on the owner's PC. If the fallback only restores Classic, the multi-style resource/visibility path still requires runtime diagnosis.

**Default delivery:** Use the **owner's already-integrated in-editor `Pixelwolf-GGC-Updater.exe`**, not a manual source ZIP. Once the full build passes, update/launch PW Edition, check the installed commit, paint a small area, compare one native style, two visibly different native styles (50/50 and 25/75), then a custom texture. Move the editor camera near and far, toggle vegetation, save/reopen and Test Level. Capture a screenshot and updater commit if mixing still displays just one type or vanishes.

**Detailed regression notes:** [PW_WORKSPACE_SKY_GRASS_IMPORT.md](PW_WORKSPACE_SKY_GRASS_IMPORT.md).

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
| Code committed on active branch | New disappearance-correction source and regression tests through `ac7ab53c`; documentation through `0bdbfe76` before this handoff edit |
| Targeted source and UI checks | Passed for the Asset Inspector revision |
| Entire GameGuru Core Release x64 CI compile | Earlier overlay-fix code **passed** #45009785335; new disappearance correction's compile #45011851690 **pending** at last check |
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

### P0 — Fix grass regression, then test the newest editor

- [x] Patch old-grid overlap/shadow visibility, then patch follow-up **all grass disappears** regression: require `OwnsGrid()` with populated instances, use Classic fallback when empty, widen mask coverage and apply per-instance native scale/yaw. Targeted UI regression guards passed.
- [ ] Confirm full Windows x64 compile of **latest grass disappearance fix** after `ac7ab53c`; inspect latest CI, not the older overlay/Inspector build.
- [ ] Use **existing in-editor updater**, confirm installed GitHub commit, and check that enabling a second grass type does **not make all grass vanish**. Verify both styles actually appear (fallback to one Classic style is safety, not full acceptance); then custom style, camera/shadow movement, vegetation toggle, save/reopen and Test Level.
- [ ] If a *single* imported grass style still flickers with mix disabled, examine actual texture transparency and native UV atlas compatibility before changing shader/material processing.
- [x] Earlier Asset Inspector-inclusive full Windows x64 compile succeeded in run #44994266761.
- [x] Implement/source-check the **safer local build/staging path** and acceptance checklist; Windows local-script validation passed in run #44999063741. No compiled proprietary artifact has been published.
- [x] **In-engine updater already exists in the owner's current installation** (owner-confirmed and source-backed). Do not rebuild or manually ZIP-install the editor as the routine update process.
- [ ] **Next local acceptance:** use the existing PW Edition in-editor updater to build/install the new revision with its backup/confirmation flow, launch PW Edition and verify the installed commit. If the updater errors, inspect its logs before falling back to the separate local test-build route.
- [ ] Run **local editor smoke test**: launch map editor, confirm toolbar/Classic UI and docked bottom browser; open Asset Inspector; browse nested folders; preview actual PNG/DDS and stock models; rotate/zoom; test missing files; insert stock FPE and an FBX; verify placement in front of camera, undo, hierarchy, map save/reopen.
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
