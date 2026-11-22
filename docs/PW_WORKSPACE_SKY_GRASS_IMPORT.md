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
