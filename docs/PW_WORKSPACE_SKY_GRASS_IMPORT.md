# Pixelwolf Edition: docked workspace, importer previews and custom sky/grass styles

**Implementation:** 22 November 2026, development branch `work/pw-community-integration-20261121`. Source changes require a fresh compiled Windows x64 editor. Not merged into master or user-accepted.

## Docked editor

- The Scene Hierarchy, central 3D editor, Asset Browser and Inspector are parts of one ImGui dockspace. The default center/bottom split is **inside** the side panels rather than an overlapping popup.
- A revised `PWEngineDockspaceV7` layout migrates from previously saved floating layout IDs. The center and bottom dock nodes use no-tab-bar flags to prevent them being dragged away independently while retaining their splitter resize.
- The browser no longer overwrites its authoritative bottom dock ID with a dock ID learned from a mistakenly detached window.
- The original editor theme and toolbar icons should remain unaffected. Full Windows visual acceptance is still required.

## Imported-model thumbnail

- The docked DX11 preview no longer switches into Classic's 64×64 legacy bitmap before attempting a 128×128 camera-surface capture. The non-ImGui Classic path keeps its original 64×64 bitmap.
- The importer won't recycle a solid-colour or unreadable source BMP into a new asset.
- After saving a newly generated BMP, a lightweight content check rejects all-green/flat-colour captures instead of preserving misleading thumbnails. It cannot guarantee good framing, correct shader appearance or thumbnail correctness for every model.
- Owner must test an untouched Serine FBX, saved BMP and Asset Browser refresh after a fresh build. A blank BMP means capture still needs visual/runtime debugging.

## Import custom sky style

Open **Terrain Tools → Customize Sky → Import Sky Style...**, then choose the **`skyspec.txt`** file in a compatible Classic sky folder. The folder must also include a sky mesh matching its name, such as `Misty Valley/Misty Valley.DBO` or `Misty Valley/Misty Valley.X`, and its required textures/specification. The editor copies supported assets into `Files/skybank/<style>`, refreshes the sky style list and selects the new sky.

This is Classic style-folder import, **not** arbitrary HDRI or six-image cubemap conversion. The existing Classic skyspec/shader pipeline still determines sky appearance.

## Import custom grass style

Open **Terrain Tools → Customize Vegetation → Import Grass Style...**, then choose **`veg.DBO`** or **`veg.X`** in a compatible vegetation style folder with **`grass.dds`**. The editor copies supported files to `Files/vegbank/<style>`, refreshes the bank, selects the style and reloads the existing vegetation mesh and texture.

A single standalone PNG grass image is not yet converted into a Classic-style grass package. Users can author the standard folder in a 3D editor or supply an existing Classic-compatible style folder.

## Safety/limits

- No automatic overwrite of existing sky/vegetation styles.
- Installer copies files into a staging folder inside the destination bank then renames into place; failed copies clean up their staging files.
- Only regular flat folders, with style names containing letters/numbers/spaces/hyphen/underscore, and regular model/texture/specification files are supported. Rejects nested folders, reparse links, unsupported extensions, >256 files or >512 MiB. Long paths beyond the legacy `MAX_PATH` limit are rejected.
- Files are copied; source asset folders are untouched.
- Style preflight validates the expected file names and presence, but **does not** validate mesh geometry, shader compatibility or a running GameGuru render.

## Acceptance gates

1. Complete Windows x64 editor build with branch head.
2. Show resizing center/bottom splitter without floating/moving either window, including reopening a saved project and editor restart.
3. Import Serine FBX, save DBO/FPE/BMP, confirm thumbnail depicts character, then save/reopen and place asset.
4. Import a known working Classic sky folder, use it in a level, save/reopen and play Test Level.
5. Import a known working Classic grass folder, paint grass, save/reopen and test standalone game; ensure existing styles still work.
6. Confirm cancellation, missing required files, duplicate folder names and permission failures leave existing assets unchanged.

**Status:** source implementation; synthetic Windows tests and full build must be checked on the corresponding commit before claiming passes. This is not an installer release.
