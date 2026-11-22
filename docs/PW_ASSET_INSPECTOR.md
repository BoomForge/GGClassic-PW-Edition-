# Pixelwolf Edition — Asset Inspector (next build)

User request: Add a **separate popup** with the on-disk GameGuru `Files` directory tree on the **left** and an asset preview on the **right**. The permanent docked bottom Asset Browser, original Classic toolbar, scene hierarchy, and Inspector remain unchanged.

## Editor experience

- Open **Asset Inspector...** using the button at the top of the bottom Asset Browser.
- A separate, closable `Asset Inspector` window opens; it is explicitly non-dockable.
- Browse the local installed GameGuru `Files` folder and subfolders with the expanding directory tree; click any file to select it.
- **Images:** PNG, DDS, JPG/JPEG, BMP and TGA are shown from the actual image data, scaled to fit without distortion.
- **Models:** FPE native entity profiles, DBO, X, and FBX models use the actual 3D geometry. Drag with the left mouse button to rotate; mouse-wheel to zoom; use Reset View to restore the view. An offscreen rendering pass uses the existing GameGuru model thumbnail pipeline rather than a new viewport backend.
- FPE model paths and albedo textures are resolved from the FPE file when available. No dummy cube replaces a failed import; the panel reports a missing model.
- **Insert into Scene**: creates a native entity instance using the normal entity bank, placement defaults, and undo recording, positioned forward of the editor's camera and grounded at the terrain height. It does not place at world origin or use the UI mouse location.
- Selecting native DBO/X or FBX automatically creates an ordinary editable FPE/DBO copy under `Files/entitybank/user/PWInspector` before creating the scene entity; the original selected asset is not overwritten. Existing FPE entries are added directly.
- Preview resources are deleted when selecting a different file or closing the window.

## Build and testing status

The build failure reported by the updater was not caused by warning C4267/C4305. The GitHub full Windows build from 22 November showed **C2589/C2059** from Windows `min`/`max` macros in PW-GrassMix.h and M-Grass.cpp, and **C2440** from constructing `cstr` from `const char*` in M-MapFile.cpp. Those were fixed in the development branch; the standalone full x64 build passed at commit `f3dde58d` before the Asset Inspector changes.

The updater now retains compiler error lines for its failure dialog instead of displaying only the final warnings.

**Updated build verification (22 November 2026):** The Inspector-inclusive full Windows x64 CI build **passed** at source commit `4716fe95` in [run #44994266761](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44994266761). All full-compile and staged-file checks passed, but the workflow published **no downloadable build artifact**. Source compilation does not establish local runtime correctness, Steam overlay success, or an updater release.

The Asset Inspector is **source-implemented but not yet confirmed in a local interactive editor session**. Required user acceptance:

1. [x] Full Windows x64 compile at source `4716fe95` passed (run #44994266761); recheck on later source changes.
2. Open Asset Inspector and verify the persistent docked browser and Classic icons do not change.
3. Browse several nested entitybank folders, inspect PNG/DDS images and FPE/DBO/X/FBX models.
4. Drag-to-rotate and wheel-to-zoom without changing the scene camera.
5. Insert a stock FPE and an automatically converted raw model in front of the camera; check terrain height, Undo, scene hierarchy, save and reopen.
6. Test malformed/unsupported assets, missing textures, and safe cleanup on close.
7. Check custom-entity preview material relinking and export dependencies, especially native FBX material atlases.
8. Update a clean local Steam installation using the built-in updater only after successful compile and smoke test.

**Known limitation:** A raw FBX can produce a mesh with external texture references. The automatically generated native DBO/FPE may need texture relinking in the model importer if those external textures are not embedded. The Inspector never claims to repair UVs or missing material files automatically.

Do not describe this revision as a verified working installer until real editor acceptance.
