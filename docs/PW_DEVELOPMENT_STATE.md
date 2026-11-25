# GameGuru Classic — Pixelwolf Edition: Development State

**Purpose:** This is the canonical *short handoff* for continuing development in a new ChatGPT conversation. It is updated after meaningful, verified changes. Read this before treating an older chat transcript, roadmap item, or PR body as current state.

**State checked:** 25 November 2026  
**Repository:** https://github.com/BoomForge/GGClassic-PW-Edition-  
**Active development branch:** `work/pw-community-integration-20261121`  
**Draft pull request:** https://github.com/BoomForge/GGClassic-PW-Edition-/pull/1  
**Verified branch head before this documentation commit:** `930691cf84bfeb10bcc594199c272561d1a963f9` (Asset Inspector preview fixes, browser filtering and dedicated documentation; inspect GitHub for newer commits).  
**Confirmed previous Windows x64 full compile:** [run #45012104941](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45012104941), **success**, for the _still broken in live editor_ disappearance-fix revision `0bd43d92`. This demonstrates compilation does not validate multi-grass visuals. The latest **Asset Inspector blank-preview and file-filter correction** has [full Windows x64 build #45158073815](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073815) still **in progress** at `73eee8d2`; its [UI regression guards #45158073857](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073857) **passed**, and [Windows import/cleanup validation #45158073936](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073936) **passed**. Newer source/doc commits trigger additional builds; check current HEAD result before installing. Updater is already integrated into the owner's editor. **Recheck Actions before claiming the latest full build passed.** The separate Windows updater EXE is already part of the owner's build; it also [compiled successfully at `9248ed24`](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44999138922).

> **OWNER-CORRECTED DELIVERY FACT (22 Nov 2026): The current installed PW Edition ALREADY INCLUDES AN IN-ENGINE UPDATER.** Never describe this updater as a future feature, or default to downloading source ZIPs and running `START_PW_TEST_BUILD.cmd` every update. The editor's update command launches the installed `Pixelwolf-GGC-Updater.exe` source-building helper. Distinguish this **existing, owner-confirmed capability** from whether the **latest revision** has passed actual installation and gameplay acceptance; CI compilation is not a local runtime test.

## 25 November 2026 — multi-grass STILL BROKEN, proof-first DX11 diagnosis (P0)

**Owner's fresh live result:** After the latest updater, **ONLY ONE grass type still appears**. Prior source edits did not deliver multi-species rendering. The owner explicitly requested deep code analysis **without further guesses**. Do not claim the issue is solved or substitute another passing static test for actual rendered grass. The README Discord archive must continue to list multi-grass as broken until confirmed visually.

**Code-established design failure:** The implementation is not a proportional multi-grass engine: Classic's full-density native vegetation draws the primary type; `PWGrassMix::Refresh` adds only up to **320 separate extra objects** and discards selections of the native type. This cannot honour the requested 50/50 or 25/75 mix. The actual underlying Dark Basic `InstanceObject`, source texture, transparent sorting, visibility and render path were read directly. **Whether the secondary DBO draws any pixels on the user's GPU remains unknown**, and changing shaders/scales again without evidence would repeat the previous mistakes.

**Evidence-generating code committed on development branch:** `M-Grass.cpp` now counts real painted-mask hits, native-vs-extra sample picks and failed extra-object allocations. `M-Terrain.cpp` displays those stats with **Copy grass diagnostics** and an opt-in **Diagnostic: compare magenta cube and second grass model**. The test draws an opaque magenta cube plus a large actual secondary grass **clone** directly in front of the current camera (uses real camera look vector, not guessed pitch). It is not part of the saved level, is cleaned up on unload/disable, and follows shadow-pass visibility. The normal grass renderer and Classic fallback remain untouched. Targeted Python assertions and deterministic C++ mix-distribution checks were updated. Diagnostic implementation commits from [`0d97b6a9`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/0d97b6a9389ee5c47d141e74a429f71e5d88eb93) through [`57976ac5`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/57976ac54b8fa0a07e0628582bf19c533c8e97a7).

**Install/build gate:** Recheck latest branch x64 CI for the diagnostic code after the last source/test change; never say compiled based on an earlier SHA. Use the existing in-editor updater ONLY after full Windows x64 CI is green. Do not distribute new ZIPs. This is a **diagnostic stage, not a finished feature**.

**Decision tree from one real owner test:** Choose two clearly distinct stock grass styles, enable **Show additional grass types**, then enable the magenta-cube/model diagnostic. Copy the counters and observe the viewport. Cube missing = camera/render path. Cube visible, grass clone missing = model material/shader/texture or geometry. Both visible but scattered grass missing = placement/instancing/coverage. If painted/extra picks are zero = missing assets/mask/selection; investigate before changing GPU code. Once at least two species visibly render, replace the non-proportional Classic+320 overlay with a genuine weighted and performance-bounded multi-style renderer.

**Source of detailed procedure and history:** [PW_WORKSPACE_SKY_GRASS_IMPORT.md](PW_WORKSPACE_SKY_GRASS_IMPORT.md). No code-only test can prove the graphics are visible on the owner's actual editor.

## 24 November 2026 — deep multi-grass renderer correction (P0; awaiting visual acceptance)

**Owner request:** After several failed updates, **multi-grass still does not work**. Investigated source from saved FPM config through editor controls, native vegetation setup, painted-mask refresh, DX11 object effects and the Dark Basic `CObjectsC.cpp` instance/alpha render implementation. This supersedes earlier patch attempts that only changed masks, ownership or shader assignment.

**Source-level root problems found and corrected:**
- Saved two-style configurations always reset the PW extra-style renderer **OFF** on load, so only the one-style Classic grid was visible. A newly selected second style also remained OFF unless the user found the separate experimental checkbox. Valid saved mixes now resume the overlay, and newly activated mixes automatically enable the extra styles. The checkbox is now **Show additional grass types**, retaining immediate OFF fallback.
- Classic's native vegetation style only followed the mix selection when precisely one slot was active. With multiple choices it could show an unrelated previous style. The first enabled mix slot is now used as Classic's base both when editing and when initialising a saved mixed-grass map.
- Extra grass instances did **not respect `VegWidth_f`/`VegHeight_f`**, and had a **double scale** bug: `InstanceObject` multiplies its source's scale by the instance's scale in the underlying Dark Basic 3D engine. Mesh dimensions are now measured, converted to safe world-size percentages by `PWGrassMix::ComputeInstanceSizing`, and applied **once** to visible instances; hidden mesh sources remain at 100%.
- `SetObjectTransparency(...,6)` uses a fixed 0xCF (~81%) alpha-test cutoff; soft-edged blade textures could be mostly clipped. Source + instances now use blended transparency mode 2.
- Secondary mesh/image IDs previously came from unrelated importer ranges and could collide with live editor allocations. PW grass now probes isolated free ranges starting at 230000 (object) and 125000 (image).
- PW mask sampling ran before native paint/terrain refresh in `grass_loop()`. It now runs after the native update, using current painted vegetation.

**Committed branch code:** `M-Grass.cpp`, `M-Terrain.cpp`, `PW-GrassMix.h`, targeted Python UI regression checks and isolated C++ `test_pw_grass_mix.cpp`. Feature change through [`31c4d9bd`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/31c4d9bd42aed20e8c3e62206f81cfb6f4c54290), tests through [`41a6134e`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/41a6134e3400166b5b7badc643ea02c57b2bd468). Full technical root-cause log and test steps: [PW_WORKSPACE_SKY_GRASS_IMPORT.md](PW_WORKSPACE_SKY_GRASS_IMPORT.md).

**STATUS: Source repair attempt, not yet visually confirmed.** Targeted tests for earlier corrective commits passed; recheck latest **full Windows x64 build** and **UI + C++ grass-sizing validation** at current HEAD before installing. The actual owner has not yet tested this correction. Keep native Classic grass rendered as the reliable fallback, maintain multi-grass opt-out, and protect the prior Asset Inspector repairs.

**Important remaining architecture gap:** The current secondary renderer adds at most **320 independently instanced blades** alongside the full-density Classic first style. It is not a true proportionately mixed native vegetation grid; 50/50 and 25/75 selections are **not guaranteed** on screen. First validate that **two clearly different grass types appear at all**, with camera movement, painted mask updates and save/reload. Then address proportional distribution, fog/wind/shadow and performance. Do not tell the owner or Discord that multi-grass is fixed until the actual editor demonstrates it.

**Next practical sequence:** Check successful Windows x64 full compile; only then use the **already installed in-editor PW updater**. In a disposable map: paint a dense area, choose two visually distinct native styles, verify **Show additional grass types** is ON, and capture both viewport and the source/instance counts plus mesh-size diagnostics. OFF must immediately restore Classic-only without disappearing vegetation. If counts exist but no extra blades are visible, investigate actual shader/material rendering rather than repeating static tests. The README Discord changelog still correctly lists multi-grass as unfinished.

## 24 November 2026 — Discord development reports archived in README

**Owner instruction:** Keep concise public-facing Pixelwolf Edition Discord development updates in the repo's **README.md** as **separate dated historical entries** rather than overwriting or combining posts. This supports future requests to summarize changes and draft fresh Discord posts using the latest handoff and prior published reports.

**Verified implementation:** Added `## Pixelwolf Edition — Discord development updates` above the existing feature tracker in the active development-branch README, with the user's full original **22 November 2026 development overview** and the shorter **24 November 2026 Discord update**, individually headed and listed newest first. Preserved existing README tracker. Future changes should add new dated entries at the top; do not silently rewrite older dated posts. README commit: [`1b8052e7`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/1b8052e758f4bd7ddfbe946bef135a9c8c3d1fb2).

**Required factual distinction for future Discord posts:** Source merged/committed, GitHub Windows compilation, and owner-tested runtime behaviour are distinct statuses. Never say **multi-grass works**; owner explicitly confirms it still does **not** work. The latest Asset Inspector indexing/search source revision passed Windows x64 CI but has not yet been performance-accepted by the user.

**Links:** [README Discord archive](../README.md#pixelwolf-edition--discord-development-updates); [Asset Inspector handoff](PW_ASSET_INSPECTOR.md).

## 23 November 2026 — owner accepted preview repairs; performance and search are now P0

**Owner live acceptance:** The latest Asset Inspector rotation and centring update **"worked really well"**. Preserve drag capture, camera framing, native GameGuru toolbar and permanently docked bottom browser. Owner identified **severe lag while the Inspector was loading** and requested a more focused browser: **only DBO and X models**, **one entry per same-name DBO/X pair** (DBO preferred), **names without file extensions**, plus **a search field in the viewer**. This updates the previous broader popup-listing requirement. Keep internal texture lookup and FBX support in the *separate importer*; they need not appear as selectable assets in this model Inspector.

**Code changes on the same active development branch:**
- [a9d3055e](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/a9d3055e59626a96e419431157329eb980f65725): bounded incremental filesystem scan rather than a synchronous recursive walk of up to 100,000 items; indexing progress without freezing the editor; index only model filenames, with no preloading of models. Case-insensitive global model search limited to showing first 200 hits; extensionless names and same-directory DBO-over-X deduplication. Measures each selected model's loading time.
- [b60601ad](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/b60601ad5d805e67e1aedaf335e00f5d6594de06): reuse a completed model index after closing and reopening the popup, and release interrupted scan handles. **Refresh Models** re-indexes as needed.
- [a1f30b9c](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/a1f30b9ce2ecb95a132d747b006c202bd9aa3976): linear duplicate elimination instead of repeated vector erases.
- [1a8e3963](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/1a8e3963efa1fb898c40dd3e60da9ed8741cb2dc): updated static UI guards. The Inspector also reuses a compiled preview shader between model selections, releasing it when the popup closes.
- Detailed acceptance and superseded scope are in [PW_ASSET_INSPECTOR.md](PW_ASSET_INSPECTOR.md). This section supersedes older popup docs that listed selectable FBX and texture images.

**Verification gate:** Source checks do not establish a measured performance improvement on the owner's PC. Latest revision must pass a **full Windows x64 compile** before using the **existing built-in PW Edition updater**; then owner should verify no initial freeze, searchable unique DBO/X model names, quick repeated open, correct model preview/rotation/zoom/insertion, and per-model load timing. Do not claim performance solved until runtime verification.

**Next three actions:** (1) check HEAD CI and repair any build/test failure; (2) install via the *existing in-editor updater* once CI is green and run acceptance; (3) investigate any remaining slow individual model using the new load-ms readout and actual filename/path. Mixed-grass remains a separate unfinished feature.

## 23 November 2026 — newest LIVE Asset Inspector regression feedback and source changes

**User tested the updated editor** and now confirms **some DBOs do show** in the Asset Inspector. This supersedes the earlier non-specific "nothing changed" report: real model rendering is partially present. Three new issues remain: (1) mouse-dragging the 3D preview **moves the window instead of rotating the model**; (2) model/preview framing is not centred; (3) other DBOs still fail to render. These are unfinished acceptance failures; do not describe the Inspector as visually verified or production-ready.

**Committed source-level fixes:** [`39afb780`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/39afb780f6fbfea49a207e6a4edcbca852e2d82c) uses ImGui InvisibleButton mouse capture around the actual preview so left drag acts on the model, not the popup; [`757bcbf7`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/757bcbf7b7c3c847c1d44aea7fa900fe2145a7bc) frames raw models around their rotated scaled bounds and uses the spatial diagonal for safe fit; [`0742de6a`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/0742de6a098d0e9596384c1181c1ac8c7f1aed7a) centres the preview square in the right pane. [`4480c218`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/4480c21834300da0d9a6407d4b4b0c172ea0862a) adds static regression checks of those paths. The prior UI regression run for this code [#45166595627](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45166595627) **PASSED**. Full Windows x64 run [#45166606100](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45166606100) **queued** when checked; recheck latest CI at HEAD before installing. These are **unverified runtime attempts**, not an accepted fix.

**Immediate next three actions:**
1. Verify completed full x64 build result for latest revision, fix compiler regressions if any, retain the draft development PR.
2. Through **owner's already-integrated PW Edition updater**, install after green CI, then test on a known-good DBO: the image must remain fixed as mouse dragging spins the model, mouse wheel zooms, and camera keeps geometry centred on reset/rotation.
3. Investigate specific DBO preview failures using two file names/paths (one failing, one working) and the preview status/screenshot. Inspect material/shader, mesh validity and preview capture path; do not infer all models work from successful compilation. Other grass-mix work remains outstanding.

See [PW_ASSET_INSPECTOR.md](PW_ASSET_INSPECTOR.md) for implementation details.

## 23 November 2026 — latest owner report: updater ran, editor appears unchanged

**Owner report (not independently verified):** After retrying the built-in updater following the Asset Inspector C2059 fix, the owner reports **"I updated but nothing changed"**. Treat this as an unresolved **runtime/installation acceptance failure**, not proof that the Asset Inspector visuals are fixed, and not proof the updater failed. The specific unchanged behaviour and installed source commit were not supplied.

**Independent build truth:** The full Windows x64 CI build for latest feature/doc revision `0a6fd773` **PASSED** at [#45162913299](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45162913299), along with the UI guards [#45162913169](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45162913169). Passing CI does not verify the user's executable or 3D model pixels.

**First diagnose delivery, then rendering:**
1. Confirm that the updater ended with the explicit **"PW Edition installed and SHA-256 verified"** result, as opposed to merely reporting a successful build or being cancelled at the **"Install now?"** confirmation. The updater copies `Guru-MapEditor.exe` to the specifically selected Steam GameGuru directory; its **`GameGuru.exe` Steam launcher is deliberately not replaced**.
2. Open that same directory's `Launch-PW-Edition.cmd` (or its `Guru-MapEditor.exe`) **directly**, not Steam's normal Play command, which may launch the stock editor. Check its `pw-installed-commit.txt` against the currently published branch HEAD `0a6fd773` or newer; the updater writes this marker only as part of a verified installation.
3. The original docked Asset Browser is intentionally unchanged. In that browser click **Asset Inspector...** to open the *separate* popup, then check whether the popup has **Refresh Files** and **Refresh 3D Preview**, excludes FPE/thumbnails, and renders an actual stock DBO/X or FBX. If source commit is current and the popup is still blank/noisy, debug preview capture/filtering as a **real runtime regression**, not another assumed updater issue.
4. Request the last **SUCCESS / Commit / Editor** log lines from `%LOCALAPPDATA%\\Pixelwolf\\GGC-Updater\\logs\\update-*.log` only if path/commit verification is inconclusive. Do not ask for a fresh ZIP/manual install as the default.
5. Preserve separate mixed-grass unfinished issue; this report does not establish whether the grass test was run.

**Next engineering gate:** Prioritise exact installed-commit + launched-executable path verification. If the confirmed current PW editor still shows a blank model or unfiltered tree, trace actual offscreen render and the selected asset/preview status rather than repeating static checks; mark visual acceptance blocked until real screenshots establish results.

## 23 November 2026 — OWNER UPDATER COMPILE FAILURE AND CORRECTION

**Latest corrective action (supersedes older in-progress build snapshots below):** The owner ran the **already-integrated in-editor updater** and received a pre-install MSBuild failure. No files were installed. The updater displayed C2059 syntax errors against DarkEXE.vcxproj but lacked the original compiler source location. Independently fetched GitHub full x64 job logs for runs [#45158348201](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158348201), [#45158211244](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158211244) and [#45158073815](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073815): **all FAILED** with the identical first source error at `GameGuru Core/GameGuru/Source/M-GridEdit.cpp(21342,76): error C2059: syntax error: ')'`. It affected Map Editor and Lightmapper compilation, and is an upstream source defect, **not evidence of a broken Windows setup**.

- **Code correction committed as [`ade4a423`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/ade4a423a67a401f37ed87b1ce0c6fdcbcd41e79)**: removed one extraneous `) ` from the new Asset Inspector preview camera-radius calculation. The source was read back and the balanced expression verified.
- **Regression guard committed as [`a6ae4579`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/a6ae4579a0662917359e2aeb5ecfd9a538b0b7b1)**: the editor UI regression test checks parentheses balance in this expression to catch the same typo before release.
- **Full x64 CI validation at corrective source + guard:** [#45162762476](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45162762476) was **in progress** at this documentation update. The code-only correction had editor UI regression success [#45162574521](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45162574521); the guard-inclusive revision needs its own latest checks. Verify fresh Actions before installing.
- **Next gate:** Wait for a complete successful full Windows x64 build, then use **the existing PW Edition integrated updater** again (do not require a manual ZIP). Verify installed commit, then real model appearance/rotation/zoom and file filtering. A passing compile is not a visual fix or local installation acceptance.
- **Separate grass status:** simultaneous multi-grass remains visually unverified and is not solved by this Asset Inspector syntax fix.

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

**Checks:** UI regression suite [#45158073857](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073857) **passed** at source `73eee8d2`; import/cleanup suite [#45158073936](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073936) **passed**. Full x64 compile [#45158073815](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158073815) **failed** with C2059 in M-GridEdit.cpp; corrected at `ade4a423` and guarded at `a6ae4579`; new x64 run [#45162762476](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45162762476) requires completion. **NO actual local 3D picture has been verified.** The source code is a repair attempt, not visual acceptance.

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
| Entire GameGuru Core Release x64 CI compile | Asset Inspector source at `73eee8d2`, `930691cf`, `ac6da5b2` FAILED with C2059. Syntax fixed at `ade4a423`, guarded at `a6ae4579`; latest corrective full build #45162762476 **awaiting final result** at this checkpoint |
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
- [x] Diagnose owner's integrated updater C2059 failure in GitHub full x64 job log; fix extra parenthesis in M-GridEdit.cpp (`ade4a423`) and add balancing regression guard (`a6ae4579`).
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
