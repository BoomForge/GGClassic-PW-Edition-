# Pixelwolf Edition — Asset Inspector (next build)

User request: Add a **separate popup** with the on-disk GameGuru `Files` directory tree on the **left** and an asset preview on the **right**. The permanent docked bottom Asset Browser, original Classic toolbar, scene hierarchy, and Inspector remain unchanged.

## Editor experience

- Open **Asset Inspector...** using the button at the top of the bottom Asset Browser.
- A separate, closable `Asset Inspector` window opens; it is explicitly non-dockable.
- Browse the installed GameGuru `Files` folder and subfolders with a tree showing **only previewable model and texture-image formats**. Metadata files (`.fpe`), thumbnail images, and directories with no viewable assets are hidden. Results are indexed on opening and refreshed through **Refresh Files**.
- **Images:** PNG, DDS, JPG/JPEG, BMP and TGA are shown from the actual image data, scaled to fit without distortion.
- **Models:** DBO, X and FBX models use actual 3D geometry. `.fpe` remains readable internally for texture references and insertion but is not listed (it is a setup file). Drag to rotate, scroll to zoom, use Reset View or **Refresh 3D Preview**. Offscreen capture retains the native pipeline but uses inspector-specific shader, camera framing and elevation.
- For directly selected DBO/X models, the inspector searches sibling FPE profiles **internally** for the matching model texture, then applies it without listing the FPE. No dummy geometry substitutes for a failed model load.
- **Insert into Scene**: creates a native entity instance using the normal entity bank, placement defaults, and undo recording, positioned forward of the editor's camera and grounded at the terrain height. It does not place at world origin or use the UI mouse location.
- Selecting native DBO/X or FBX automatically creates an ordinary editable FPE/DBO copy under `Files/entitybank/user/PWInspector` before creating the scene entity; the original selected asset is not overwritten. Browsable FPE entries are retired: an inserted browsed raw model becomes a native editable FPE/DBO-backed entity.
- Preview resources are deleted when selecting a different file or closing the window.

## 23 November 2026 — partial live preview confirmed; rotation / centring / DBO regressions

**Owner's new real-editor observation:** Some DBO models now appear, confirming *partial* 3D preview rendering. However, dragging over the model moves the **entire Asset Inspector window** rather than rotating the model; some DBO files still do not display; visible models appear off-centre. This supersedes the earlier statement that *all* preview models are blank. **Do not mark 3D preview accepted** until mouse orbit, centring and several representative model files are tested.

**Source repair in active branch:**
- [`39afb780`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/39afb780f6fbfea49a207e6a4edcbca852e2d82c) wraps the model preview in an ImGui **InvisibleButton** and uses the mouse capture state `IsItemActive()` to apply drag rotation, rendering the actual preview image into that same widget rectangle. This should stop ImGui treating a model drag as a window drag, while keeping title-bar movement available.
- [`757bcbf7`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/757bcbf7b7c3c847c1d44aea7fa900fe2145a7bc) updates `RenderToPreview` to aim at the **rotated, scaled collision-bounds midpoint** instead of a fixed unrotated offset; calculates camera distance from the **full 3D size diagonal** and guards invalid bounds. Preserve stock tooltip camera path.
- [`0742de6a`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/0742de6a098d0e9596384c1181c1ac8c7f1aed7a) centres the preview square within the right-hand pane without changing the docked Asset Browser.
- [`4480c218`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/4480c21834300da0d9a6407d4b4b0c172ea0862a) updates static UI tests to guard drag-capture semantics and the geometry-centred camera path. Targeted UI regression run [#45166595627](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45166595627) **passed** on this code. The latest full x64 compile [#45166606100](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45166606100) was **queued** at the time this entry was written; check newer HEAD CI before using the integrated updater.

**Open acceptance questions:** Not all DBOs preview; still need names/paths of a failing model and a working model, actual screen capture, importer/model validity, shader and material behaviour, camera focus and drag-to-orbit/scroll-to-zoom verification. Successful compilation and static tests are not visual acceptance. Do not suggest downloading a ZIP/manual installation; use the **already integrated PW Editor updater** only after CI passes. Multi-grass remains a separate unfinished feature.

## 23 November 2026 — updated editor appears unchanged (owner report)

The owner reports **"I updated but nothing changed"** after the compiler-error repair. This does **not** count as accepted visual behaviour. The [full Windows x64 build for `0a6fd773`](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45162913299) passed, but the installed executable path, `pw-installed-commit.txt`, whether installation confirmation was accepted, and actual Asset Inspector window behaviour are not independently confirmed.

**First diagnostic:** The updater installs `Guru-MapEditor.exe` but deliberately does not overwrite Steam's original `GameGuru.exe`. Launch `Launch-PW-Edition.cmd` in the selected Steam GameGuru folder, inspect `pw-installed-commit.txt` and confirm the separate **Asset Inspector...** popup from the still-docked Asset Browser is being tested. A normal bottom browser that looks unchanged is expected. The popup should contain **Refresh Files** and **Refresh 3D Preview** and should filter unsupported formats. If the current SHA is installed and the popup's model still appears blank, treat this as a genuine remaining runtime renderer defect; do not claim the asset preview was repaired.

## 23 November 2026 — updater build failure diagnosed and corrected

The owner ran the **existing in-editor PW Edition updater**, which stopped **before installing** because of MSBuild C2059 (syntax error `')'`). The matching full Windows x64 GitHub CI logs for [#45158348201](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45158348201) and earlier attempts reveal the original location: `M-GridEdit.cpp(21342,76)`, in the new preview camera-radius expression. Both Map Editor and Lightmapper failed at this line.

- **Fix:** Removed the extra closing `) ` in `M-GridEdit.cpp`; committed as [`ade4a423`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/ade4a423a67a401f37ed87b1ce0c6fdcbcd41e79) and read the corrected source back.
- **Regression prevention:** [`a6ae4579`](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/a6ae4579a0662917359e2aeb5ecfd9a538b0b7b1) adds explicit parenthesis-balance checking for the preview camera-radius expression.
- **Build gate:** Source at and before `ac6da5b2` failed full x64 CI despite green static UI tests. The new corrective full Windows run [#45162762476](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/45162762476) was still running when recorded; check its live result before the next updater attempt.
- **Runtime gate:** A successful compile does not establish that a real DBO/FBX model appears. After green CI, update through the **already-installed in-engine updater**, then verify model visibility, rotation, zoom, images, filtered file listing and Insert into Scene. Do not use ZIP/manual-build steps by default.

## 23 November 2026 — blank preview and noisy file tree

**Owner observation:** The popup opens, but 3D previews show an empty scene rather than the model. The file tree also includes thumbnails and setup files that should not be browsed.

**Committed source changes (through `73eee8d2`):**
- Browser now indexes only previewable model extensions (`.dbo`, `.x`, `.fbx`) and image extensions (`.png`, `.dds`, `.jpg`, `.jpeg`, `.bmp`, `.tga`); hides FPE/metadata, common thumbnail and preview names/directories, and empty folders. Refresh Files refreshes the listing.
- FPE setup data remains available internally for resolving the correct texture on a directly selected model.
- Preview object allocations previously used 26400 (inside a realtime-entity ID range); preview image allocations started at 71750 (inside live editor image allocations). They are isolated to free IDs starting at 220000 for models and 120500 for images.
- Preview models receive a dedicated entity-basic shader and EntityEffectControl setup, with their own texture rebound after shader assignment and effect cleanup on release.
- Offscreen model preview camera is framed around the model at a safe elevation independent of the live terrain, and the inspector capture skips native terrain/shadow rendering while preserving the existing classic thumbnail behaviour.
- If an image capture fails, display a clear error and provide Refresh 3D Preview to retry. Existing rotation, zoom and Insert into Scene controls remain.
- Added static regression tests for file filtering, preview framing, shader, allocated IDs, sidecar textures and retry. Targeted UI regression checks passed on the earlier revision `5c1bcc6b`; check the latest results at HEAD.

**Not yet verified:** Full Windows x64 compile of the latest revision, or real on-device preview results. Build success alone does not establish that a model is visually rendered. Use the **existing in-editor PW Edition updater** only after successful CI; then inspect at least two stock model files, one supported FBX, one texture, rotation/zoom, hidden metadata/thumbnails and actual scene insertion.
## Build and testing status

The build failure reported by the updater was not caused by warning C4267/C4305. The GitHub full Windows build from 22 November showed **C2589/C2059** from Windows `min`/`max` macros in PW-GrassMix.h and M-Grass.cpp, and **C2440** from constructing `cstr` from `const char*` in M-MapFile.cpp. Those were fixed in the development branch; the standalone full x64 build passed at commit `f3dde58d` before the Asset Inspector changes.

The updater now retains compiler error lines for its failure dialog instead of displaying only the final warnings.

**Updated build verification (22 November 2026):** The Inspector-inclusive full Windows x64 CI build **passed** at source commit `4716fe95` in [run #44994266761](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44994266761). All full-compile and staged-file checks passed, but the workflow published **no downloadable build artifact**. Source compilation does not establish local runtime correctness, Steam overlay success, or an updater release.

The Asset Inspector is **source-implemented but not yet confirmed in a local interactive editor session**. Required user acceptance:

1. [x] Full Windows x64 compile at source `4716fe95` passed (run #44994266761); recheck on later source changes.
2. Open Asset Inspector and verify the persistent docked browser and Classic icons do not change.
3. Browse nested entitybank folders: only DBO/X/FBX models and PNG/DDS/JPG/BMP/TGA images are visible; no FPE, setup text, thumbnails or empty directories. Verify model textures from internal sidecar metadata.
4. Drag-to-rotate and wheel-to-zoom without changing the scene camera.
5. Insert a stock FPE and an automatically converted raw model in front of the camera; check terrain height, Undo, scene hierarchy, save and reopen.
6. Test malformed/unsupported assets, missing textures, and safe cleanup on close.
7. Check custom-entity preview material relinking and export dependencies, especially native FBX material atlases.
8. Update a clean local Steam installation using the built-in updater only after successful compile and smoke test.

**Known limitation:** A raw FBX can produce a mesh with external texture references. The automatically generated native DBO/FPE may need texture relinking in the model importer if those external textures are not embedded. The Inspector never claims to repair UVs or missing material files automatically.

Do not describe this revision as a verified working installer until real editor acceptance.
