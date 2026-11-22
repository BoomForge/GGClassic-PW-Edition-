# PW Edition — Texture-Only Sky and Grass Import + Weighted Grass Mix

**Corrected by owner: 22 November 2026.** This replaces the earlier
FBX/DBO/X environment-import workflow. Source is committed to the development
branch; Windows x64 full compile and visual/runtime acceptance for this
revision are still required.

## Single-image environment import (user-approved workflow)

In **Terrain Tools > Customize Sky**, keep the current native sky style (or let the engine choose a valid default) and click **Import Sky Style...**. Select **only an image**.
The engine automatically:

1. Clones the selected sky's pre-existing GameGuru geometry, skyspec/shader
   settings, and compatible supporting files into a new skybank style.
2. Copies the selected image into that new style as `pw_sky_texture.<ext>`.
3. Applies that texture to the cloned sky object when the sky loads.
4. Generates a safe, unique style name derived from the image file name and
   selects the new style.

In **Terrain Tools > Customize Vegetation**, use the current native grass style (or let the engine choose a compatible default) and click **Import Grass Style...**. Select **only an
image**. The engine automatically clones the selected vegetation mesh and
configuration, copies the selected texture as `grass.<ext>`, saves the new
vegbank style and makes it active.

**There is no imported mesh, FBX/GLB/DBO/X selection, model-importer screen,
manual texture-material setup, or scale/yaw slider in either workflow.**
The user supplies only the image; the current native style, or an automatically
chosen compatible in-engine template, determines the geometry, UVs, shading and default scale/orientation. The engine never
overwrites the original style. PNG, DDS, JPG, JPEG, BMP and TGA are accepted.
Images still need to be compatible with the chosen native template's UV atlas
layout to look correct; the engine does not reconstruct UVs or invent geometry
from arbitrary images.

Implementation:
- `PW-TerrainStyleUI.h`: image-only picker and automatic activate/refresh.
- `PW-TextureStyleClone.h`: bounded, validated staging/copy of native style,
  generated collision-safe names, template file preservation, and image copy.
- `M-Sky.cpp`: applies custom selected image to the cloned sky geometry.
- `M-Grass.cpp`: loads selected texture format in native and mixed grass paths.
- `tools/test_pw_style_import_win.cpp`: clone/texture/naming/security checks.

## Multiple painted grass types — unchanged

In **Terrain Tools > Customize Vegetation > Mix Multiple Grass Types**,
choose up to **four** existing grass styles. Each row has enable/disable,
style selector, and **Spread weight (%)** from 0 to 100.

The mix normalises the active weights, picks a style per painted grass sample
with deterministic seeded distribution, and renders each style's actual
existing mesh and texture atlas. With zero or one active style, the native
single-style vegetation grid remains in use. The bounded multi-style path
uses up to 320 camera-near instances as an initial performance safeguard.
The weights and choices are saved in optional `pw_grassmix.cfg` inside the
FPM. The standalone asset collection now includes each enabled mixed grass
style folder, not just the one currently selected; real packaged-game playback
remains to be tested. Older maps continue to use Classic single-style grass.

This is distribution control, **not** a user-side mesh import or shader-only
colour blend.

## 22 November 2026 editor regression — mixed grass shows one style / flickers

**Owner report (runtime observation):** The UI allows several grass choices, but the editor appears to display only the current/custom grass texture across the area, and custom/mixed grass flickers.

**Source inspection confirmed an overlapping-renderer defect:** Classic's original **single-style** `ShowVegetationGrid()` was invoked after building and refreshing the PW multi-style instances and again when restoring the scene after cascade-shadow passes. Both systems could paint the same areas, making the current native style dominate and producing z-fighting/flicker.

**Fix committed to the active development branch:**
- `M-Grass.cpp`: keep the native vegetation grid hidden when a mix is active, restore Classic only if a required mixed asset is unavailable, respect no-grass mode, and provide per-instance hide/show that mirrors the shadow rendering pass.
- `M-Terrain.cpp`: guard editor vegetation rebuild/refresh and post-shadow native grid restoration; temporarily hide and restore **mixed grass objects** around shadow passes; reset mixed objects when the editor's Display Vegetation toggle is disabled.
- `M-GridEdit.cpp`: prevent startup from re-showing the native grid over the mix.
- `PW-GrassMix.h`: declare the visibility handler. `tools/test_pw_editor_ui_regression.py` and CI now guard these ownership paths.
- The actual mixed renderer already loads **distinct mesh and atlas image IDs per selected style** and uses the deterministic weighted distribution. The fix is to draw only the appropriate renderer, not to reuse one texture on all options.

**Verification status:** Source fixes committed; Windows x64 compilation and actual visual acceptance remain separate gates. This is **not yet confirmed** to eliminate flickering on the owner's machine.

**If a single custom style still flickers with the mix entirely disabled:** test its selected image's transparency/alpha and whether it matches the native grass template's atlas UV layout. An opaque seamless ground texture is not automatically an alpha-cutout grass-blade atlas, and this would require a separate import/texture compatibility fix rather than treating it as the mixed-grid overlap.

**Local acceptance via the already-integrated in-editor updater (not manual ZIP):** After successful CI, use PW Edition's existing updater, then inspect: native single style with mix off; two distinct grass types at 50/50, then 25/75; a new custom style alongside stock grass; an existing painted area while moving the camera; toggle Display Vegetation; test level, return to editor, save/reopen. Confirm distinct meshes and textures without z-fighting. Record the commit reported by the updater if it fails.

## Follow-up 22 November — selecting a second grass type hides everything

**Observed by owner after the previous rendering fix:** When more than one grass style is selected, the vegetation disappears entirely. This is a separate regression from the original texture-overlap/flicker.

**Further source defects found and patched:**
1. `PWGrassMix::Ready()` meant only that style models/images loaded successfully, **not** that any grass instances were populated. All editor/shadow/startup hiding gates now use `PWGrassMix::OwnsGrid()`, which is true only after real mixed objects have been placed. If none were placed, the editor continues to display the original Classic grid instead of hiding *all* grass.
2. `grass_init()` previously hid Classic immediately if a second style was merely configured, before sampling any painted vegetation. It now keeps Classic visible until the mixer genuinely takes ownership.
3. The old mixed-grid search was only 21×21 world cells around the camera and sampled one pixel from each 2×2 painted-mask cell. In the editor, the camera can be high and away from the painted grass, and small brush strokes can miss the sampled pixel entirely. Sampling now expands outward up to a 48-cell/2400-unit radius, reads all four mask pixels per cell, prioritises near-camera cells first, and still limits allocated mixed objects to 320.
4. `InstanceObject` creates **independent per-instance transforms**; it did not copy the source grass object's native 200% scaling or template yaw. Each mixed instance now explicitly receives its own saved style scale and rotation.
5. Static regression checks now assert populated-instance ownership, old-grid fallback, painted-mask coverage, distinct per-style assets, native grass size/yaw and shadow-cycle behaviour.

**Checks / limitations:** An earlier full Windows build of the overlap fix passed, but the **new disappearance correction is not yet confirmed in a real editor**. Check GitHub Actions for the latest full x64 compilation and regression results. Even passing CI cannot prove graphics are visible. The next owner test must use the **already-installed, in-editor PW Edition updater**, then compare one type, two different native types at equal weights, and a custom style. Verify no disappearing grass, no texture domination, no flicker, and consistent save/reload. If still blank despite the fallback, capture which style names and screenshot of the scene so the remaining object/camera/render path can be traced.

## Required acceptance before an install is called ready

1. Compile full Windows x64 editor at the latest branch head.
2. Select a stock sky, import one PNG, verify the original sky is unchanged,
   and verify the new sky uses the same geometry with the selected image.
3. Repeat sky creation from an alternate builtin sky and with DDS/JPG; inspect
   UV orientation, lighting, day/night behaviour, save/reopen and standalone.
4. Select a stock vegetation style, import a PNG/JPG atlas and verify the new
   grass retains its pre-existing geometry and actually uses the image.
5. Test two to four different cloned/native grass styles at varied weights;
   painted areas only, save/reopen and standalone.
6. Check transparency, wind, shadowing, fog, FPS and VRAM (especially with
   the 320-instance mixed-grass cap).
7. Verify no FBX/mesh controls appear for **sky/grass texture import**.
   GLB remains disabled in the separate **entity model importer**.
8. Test cancellation, invalid texture files, missing built-in template,
   duplicate image names and no loss of stock assets.

**Status:** source implementation staged; targeted Windows tests and full
runtime editor/game verification remain necessary. Do not describe this
as a tested installer yet.
