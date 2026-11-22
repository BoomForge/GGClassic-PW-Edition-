# GameGuru Classic — Pixelwolf Edition: Development State

**Purpose:** This is the canonical *short handoff* for continuing development in a new ChatGPT conversation. It is updated after meaningful, verified changes. Read this before treating an older chat transcript, roadmap item, or PR body as current state.

**State checked:** 22 November 2026  
**Repository:** https://github.com/BoomForge/GGClassic-PW-Edition-  
**Active development branch:** `work/pw-community-integration-20261121`  
**Draft pull request:** https://github.com/BoomForge/GGClassic-PW-Edition-/pull/1  
**Verified branch head before this documentation commit:** `e0dcd44e2eccb9de19ced71b90789562aae298c4` (test-script and CI changes; latest source-level editor functionality remains from `4716fe95`).  
**Last independently confirmed full Windows x64 CI:** [run #44994266761](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44994266761), **success**, for editor source `4716fe95`; all compile and staged-file verification steps passed. No workflow artifact was published. New checks on subsequent script-only commits may be queued or in progress: always check Actions before citing the most recent result.

> **Important distinction:** Successful CI compilation is not a verified Steam overlay, runtime-tested editor, release ZIP, or functioning in-engine updater. Do **not** tell the owner to use the updater for this revision until a testable package is assembled, smoke-tested and verified. Avoid claiming a feature works merely because its source was committed.

## Product intent and hard requirements

- Creator-friendly, fast, familiar GameGuru Classic world-building workflow, with early-2000s game-making simplicity, retaining the Classic visual style and icons rather than replacing it wholesale.
- Engine for **Adventuring for Dummies**: initially just a **very small Merravale exterior town**; preserve the broader fantasy/dungeon-crawling story but defer RPG systems, complex NPC work and quests until the town-building tools are usable.
- Editor layout: scene hierarchy on left, viewport in centre, Inspector/properties on right, **permanently docked** asset browser along bottom. Do not create an unwanted duplicate floating browser.
- Hierarchy supports groups/nested movement; Lua Studio is available; maintain responsive asset placement, original toolbar icons, and editor UI linkage. Owner previously marked these issues completed, but new revisions still require regression checks.
- **FBX is the supported importer to test. GLB import was explicitly disabled/deferred** after failed use; never promise current native GLB support. A Model Bridge frontend having GLB preview code is not equivalent to GameGuru GLB import.
- **Custom grass/skybox import is texture-only.** The user chooses a single image. The engine clones an existing native grass or skybox template and applies the image automatically. It is **not** an FBX/model importer and should not present mesh, scale, yaw or manual setup options. Support several grass types with a weighted spread/mix control.
- Support both PNG and DDS without breaking Classic DDS compatibility.
- Keep `master` protected; implementation is in the active development branch and draft PR until tested. Do not distribute proprietary upstream GameGuru media or binaries without checking rights.

## Most recent feature: Asset Inspector (source implemented, full CI compile passed; runtime unverified)

Specification: [PW_ASSET_INSPECTOR.md](PW_ASSET_INSPECTOR.md).

Owner request:
1. Separate **Asset Inspector** popup accessible from the docked Asset Browser.
2. Actual installed `Files` folder/directory tree at left.
3. Right pane previews actual image data (PNG/DDS/JPG/BMP/TGA) or real FPE/DBO/X/FBX model; rotate by mouse drag, zoom by wheel.
4. **Insert into Scene** creates a normal editable GameGuru entity in front of the editor camera, at terrain height, using native undo/placement flow.
5. Imported raw models use a separate user-controlled FPE/DBO copy; do not overwrite the original. Preview resources should be cleaned up.

**Source status:** Implemented and targeted UI regression checks passed. Initial compilation blockers in `PW-GrassMix.h`, `M-Grass.cpp`, and `M-MapFile.cpp` were fixed; updated updater diagnostic handling now aims to show actual MSBuild errors instead of trailing conversion warnings. Full x64 build containing the Inspector passed in run #44994266761 at `4716fe95`.

**Not verified:** That the popup opens on the owner's Windows editor; 3D geometry/texture fidelity; mouse orbit/zoom; insertion location; undo; save/reopen; raw FBX material copying/relinking; local installer. Do not turn a successful CI build into a runtime acceptance claim.

## Current delivery and build truth

| Stage | Verified state |
| --- | --- |
| Code committed on active branch | Yes, through `e0dcd44e` before this handoff edit (latest editor code was already compiled at `4716fe95`) |
| Targeted source and UI checks | Passed for the Asset Inspector revision |
| Entire GameGuru Core Release x64 CI compile | **Passed** — run #44994266761 |
| Compiled editor included as downloadable artifact in that CI run | **No** — artifact list empty |
| Local Windows editor test / original Steam overlay | **Not verified** for this revision |
| Updater install and restart, end-to-end | **Not verified** |
| Production-ready release / PR merged to master | **No** — PR #1 is draft |

A successful CI run resolves the previous *build failure*. It does **not** mean the user has a downloadable new update.

## Local Windows test-build preparation (22 November 2026)

**Source-only implementation and checks (not an owner-tested build):**
- Updated [`tools/pw-local-test.ps1`](../tools/pw-local-test.ps1) to validate that Steam source contains Files and GameGuru.exe; reject unsafe destinations (Steam, repository root/unrelated subfolders, build output, test-directory reparse point); check required PW files and the compiled editor branding before staging; record the source commit and editor SHA-256; verify the staged editor hash. `-SkipBuild` requires a matching source-commit marker rather than silently reusing an unrelated binary.
- Updated [local build instructions](PW_LOCAL_WINDOWS_TEST_BUILD.md) and the generated `PW-TESTING-README.txt` to prioritise Asset Inspector, FBX thumbnail/placement, sky/grass texture-only cloning, scene save/undo/docking and explicit GLB deferral.
- Existing root [`START_PW_TEST_BUILD.cmd`](../START_PW_TEST_BUILD.cmd) remains the one-click launcher: clone/download the current branch, install local VS2022 C++/MFC prerequisites, double-click and select your licensed Steam Classic folder. Script compiles on the **owner's Windows PC** and stages an isolated `PW-TestInstall` copy. No licensed engine binaries/Steam media are published as CI artifacts.
- [Windows local-script CI](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/workflows/pw-local-build-script-validation.yml) checks PowerShell syntax and rejects harmful destinations using synthetic placeholder Steam files. The first negative-path test harness left an expected non-zero child exit code (run #44999006585); the correction at `e0dcd44e` **passed** Windows PowerShell syntax, destructive-destination rejection and Steam import-metadata checks in [CI run #44999063741](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44999063741). This is not a local editor or Steam acceptance test.
- This is **not** evidence of a successful local Steam overlay, editor launch or visual preview. It is a safer path to obtain those results without distributing the proprietary executable.

### Local next-step instructions

1. Fetch the current development branch and double-click `START_PW_TEST_BUILD.cmd`; select the *licensed Steam GameGuru Classic* folder. Use a machine with Visual Studio 2022 Desktop C++/MFC dependencies. **Do not use the old in-engine updater for this test.**
2. Start `PW-TestInstall\Guru-MapEditor.exe` **directly**, not Steam's Play action. Consult `PW-TestInstall\PW-TESTING-README.txt`, which records the built commit, editor SHA-256 and detailed Inspector / import checks.
3. Bring back the first failed step, exact error and tested source commit. Fix reproducible faults in the branch, rerun CI, then repeat the local test. Only consider updater acceptance after passing actual editor/runtime tests.

## Next actions, in priority order

### P0 — Deliver and test this revision safely

- [x] Full Windows x64 compile of source including Asset Inspector succeeded in run #44994266761.
- [x] Implement/source-check the **safer local build/staging path** and acceptance checklist; Windows local-script validation passed in run #44999063741. No compiled proprietary artifact has been published.
- [ ] **Owner-side build and launch still required:** run `START_PW_TEST_BUILD.cmd` on the licensed Windows PC, then launch `PW-TestInstall\Guru-MapEditor.exe` and confirm identity/runtime. Do not install over Steam or publish proprietary binaries.
- [ ] Run **local editor smoke test**: launch map editor, confirm toolbar/Classic UI and docked bottom browser; open Asset Inspector; browse nested folders; preview actual PNG/DDS and stock models; rotate/zoom; test missing files; insert stock FPE and an FBX; verify placement in front of camera, undo, hierarchy, map save/reopen.
- [ ] Confirm FBX thumbnail generation, texture-path handling and scene placement; verify no failed imports create ghost assets.
- [ ] Confirm cloned skybox and grass textures with multiple grass styles survive save/reopen and exported game. Confirm existing native mesh/UV behaviour.
- [ ] Only after local acceptance, test updater on a **disposable** PW installation, including integrity checks, error handling, rollback and optional restart.

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
7. Do not reintroduce owner-rejected behaviour: undocked duplicate browser, altered Classic icons, GLB presented as working, or mesh-based custom grass/skyboxes.
8. When owner requests "continue GG Classic" in a fresh chat, use the repository first and proceed from the next uncompleted gate; ask only for genuinely missing inputs that block actual work.

### Suggested resume message

> Continue GameGuru Classic — Pixelwolf Edition from `docs/PW_DEVELOPMENT_STATE.md` on the active GitHub development branch. Check latest commits and CI before making changes; update and read back that file after completing work.
