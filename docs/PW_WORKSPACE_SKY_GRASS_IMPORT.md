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

## Follow-up after repeated failure: isolate experimental renderer (22 November 2026)

**Third owner reproduction:** Even after the `OwnsGrid()` fallback, activating any second grass style still made all grass disappear in the running editor. This is definitive **visual/runtime failure** of the standalone mixed-instance renderer. Compiler, static regression and imported-asset test successes were **not** visual verification. The static tests had only checked that instance objects were created and marked visible; that does not prove the intended pixels were drawn or that the 320-object experimental preview covers Classic's vegetation grid.

**Safety + diagnosis implemented in source through `b3687530` (latest full CI pending at time of writing):**
- **Selecting multiple grass styles and weights no longer switches rendering modes automatically.** Mix choices are preserved in the project; original Classic grass remains visible as the safe/default renderer.
- The new **Preview experimental multi-grass** checkbox explicitly opts in to the alternative renderer **for diagnosis only**. It is OFF on startup and resets when loading another level. Turning it OFF releases mixed objects and restores Classic.
- Preview mode displays actual style model count, instance allocation count, placed count and per-object visible *flags*, plus reasons for missing grass mask, model or texture. This separates failed resource preparation from cases where objects exist but nothing is drawn. **Visible flags do not prove GPU raster output.**
- The original multi-grass UI remains available as configuration; **do not claim the feature is working, or silently auto-enable preview** until a real local editor test shows two distinct types.
- Preserve the **already-installed in-editor PW Edition updater** for applying this source fix after a successful full Windows compile. Do not instruct a manual ZIP install as default.
- Next diagnostic evidence to request: after updating, check that selecting two styles with experimental preview OFF leaves Classic grass visible; optionally turn preview ON in a disposable test map, capture a screenshot of the statistics and viewport, then turn it OFF. If stats say instances are placed/visible yet the scene is blank, focus on camera, object/material/shader or native vegetation API integration; don't keep modifying only mask weights and instance counts.
- **Project priority remains a real integrated multi-style grass renderer.** This safety gate intentionally does not satisfy that feature.

## Fourth owner observation — grass visible, but still only one type (22 November 2026)

**Observed:** After the prior safety gate, grass renders again, but selecting multiple types still shows one type. This is **expected for safe mode** because `Preview experimental multi-grass` is OFF by default and Classic's native vegetation grid supports one style. Saved weighted selections alone did not mean the additional style was rendered.

**Next source attempt (through `a8b68916`, Windows compile pending at write time):**
- The independently generated PW grass objects lacked an assigned **entity shader**, unlike both the Classic native grid and GameGuru's standard entity importer. In DX11 such objects can exist and be marked visible while rendering no pixels. On each *secondary* source model, use `loadinternaleffectunique` for an entity-basic/PBR-basic shader, `SetObjectEffect`, restore the per-style atlas **after** effect assignment (GameGuru's importer warns it can reset texture stages), configure the standard importer `EntityEffectControl` value, and release the effect when clearing instances.
- The preview is now an **overlay**: the single reliable Classic species remains visible through editor startup, vegetation refresh, the main grass loop and shadow pass; PW creates separately textured additional styles instead of hiding/replacing Classic. It deliberately **skips duplicating the selected native species** to reduce overlap and z-fighting.
- The on-screen diagnostics list per-style instance counts. A nonzero count and visible flag still do not prove correct rendering; only the user's visual test does.
- The preview toggle remains **OFF by default**, safe for the owner's existing project. Opting in enables experimental secondary-style overlay on a disposable map. The 320-object budget and original native density mean the 50/50 or 25/75 values are **not yet visually guaranteed** as true final mixed proportions; do not claim weighted mixing is accepted.
- Full Windows x64 CI and static regression checks must pass; then user uses the **existing built-in updater**, enables *Preview experimental multi-grass*, chooses two very visually different styles, and inspects both viewport and per-style diagnostics. If count for second style is nonzero but it remains invisible, investigate shader/vertex layout/effect constants/camera and texture atlas. Do not label the feature complete until real visual proof.

## 24 November 2026 — deep grass-renderer investigation and corrective build

**Owner request:** Multi-grass **still does not work visually**. Do not repeat the claim that selecting two styles produces visible mixed grass merely because CI passed. Diagnose the full path rather than only changing style weights.

**Primary-source findings (actual GameGuru code, not speculation):**

1. **Saved selections were not drawn.** `PWGrassMix::Load` had deliberately forced `previewEnabled = false` for every map, meaning a correctly saved two-style configuration reliably reloaded as *one native type*. The checkbox was also off when a new mix was configured. The corrected path restores the additive renderer for a valid saved mix, and enables it when a second style is first selected. The owner can still disable the extra styles using **Show additional grass types**, with unchanged Classic grass as fallback.
2. **Wrong base species.** The terrain editor only synchronised Classic's active style when **exactly one** mix slot was selected. With two or more, an unrelated previously selected native grass could dominate the screen. The base is now the **first enabled mix species** in the editor and again during grass initialization of a saved map.
3. **Incorrect world dimensions.** Classic creates its grid using the `VegWidth_f` and `VegHeight_f` sliders; the experimental standalone instances ignored those parameters and only used a fixed 200% style scale. The custom path now measures each real loaded DBO/X mesh and normalises instance dimensions against those same vegetation settings. It reports raw mesh width/height and applied scales in diagnostics, rejecting invalid/NaN geometry.
4. **Scale applied twice.** Inspected Dark Basic's `CObjectsC.cpp` implementation: instances multiply the original source object's scale by their own scale. The experimental code scaled the source and each instance, magnifying/altering the actual sizes twice. The source is now held at native **100%** and only the visible instances receive the calculated size/yaw.
5. **Alpha was cut too aggressively.** `SetObjectTransparency(...,6)` uses alpha blending **plus a fixed 0xCF (207/255) alpha cutoff** in `CObjectsC.cpp`. That can discard large portions of soft-edged grass textures. Both source and instances now use blended mode **2** without that severe hard alpha threshold; visual alpha appearance must still be tested.
6. **Temporary ID collision risk.** Previous secondary models/images allocated from importer offsets plus 4000, not guaranteed isolated from editor objects and previews. Grass now uses separate free-ID search ranges starting at **230000 for objects** and **125000 for images** (the Asset Inspector probes a different range).
7. **Stale painted-mask ordering.** `grass_loop()` previously refreshed the secondary objects **before** rebuilding/refreshing Classic terrain vegetation and bitmap data. The PW refresh now runs **after** the native update, using the latest mask and invalidation state.

**Implementation source and regression commits:**
- [c6899f89](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/c6899f89e0c4b6a8c3f320d77ed3947960177238) — style dimensions, alpha mode, diagnostic fields, isolated IDs
- [8f4b0878](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/8f4b08786bfe29965a2d8098f000d34afd5c3f94) — eliminate compounded source/instance scaling
- [02ff1bb1](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/02ff1bb17d4b7289a0d65177a45d85d3d2015419) — reorder native and PW mask updates
- [fb7b41e2](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/fb7b41e26f1b0759b0e540fc0c2ffef94080542c) and [8b8696cb](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/8b8696cbd421812792b7918770ffae5dd1b61e4c) — pure tested mesh-size calculation and adoption
- [5e7d1356](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/5e7d135671b861713009c5f50364d88a9aadeae1) — Windows unit tests for valid/invalid mesh sizes
- [d497ff08](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/d497ff08187027275d2ded20fdd5903f666134b4) and [31c4d9bd](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/31c4d9bd42aed20e8c3e62206f81cfb6f4c54290) — chosen native base in editor and on saved-map initialization
- [ebda3896](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/ebda389661ad11f2fa3e998668ad0f4d20c3eb5e) and [b583a026](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/b583a0262f38e4fb41a6b53855306057ee6a04d0) — saved/new mix activation with explicit fallback
- [41a6134e](https://github.com/BoomForge/GGClassic-PW-Edition-/commit/41a6134e3400166b5b7badc643ea02c57b2bd468) — final targeted UI guards

**Important known design limitation:** Classic's native vegetation grid remains full-density for the chosen primary style. The extra species are separate ordinary objects with a **320-instance budget**. Consequently, even if they appear, **50/50 and 25/75 spread weights are not yet visually proven to represent the requested proportions**, and this additive implementation is *not* a genuine multiple native vegetation grid. Shader, transparency, fog, wind and performance need real in-editor verification before changing the architecture further. Do not mark the feature resolved or public-ready on source inspection alone.

**Test and evidence gate:** Latest full Windows x64 compilation plus both UI and isolated C++ grass-sizing tests must pass for this revision. Then use the **existing in-editor PW Edition updater** on a disposable map (not a manual ZIP):
1. Paint a substantial grass area with two visually contrasting built-in styles selected at 50/50. Confirm **Show additional grass types** is checked (it should switch on automatically for a new mix). The native first style must remain visible and the second must be visually identifiable.
2. Capture UI diagnostics (`Overlay models`, `placed`, per-style instance counts, raw mesh size and percentage scales). Compare with the real image rather than treating visible flags as drawn pixels.
3. Move the camera and paint/erase vegetation; try 25/75, new/custom texture style, F9/test game and save/reload; the overlay must update without stale patches or vanishing grass.
4. Toggle **Show additional grass types** OFF to verify Classic still renders immediately. Measure FPS and pauses on the owner's GTX 1660 before increasing the instance cap.
5. If per-style placed counts are nonzero but the secondary isn't visible, inspect the screenshot and shader/texture mesh path next. If counts are zero, inspect missing assets, painted mask and scale diagnostics. Never declare visual success without direct owner confirmation.

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
