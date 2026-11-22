# PW Edition — Sky, Grass and Weighted Grass-Mix Import

**Status 22 November 2026:** development branch implementation. Must pass full Windows build, FBX/sky/grass runtime visual acceptance and standalone level tests before calling complete. This replaces the earlier folder-copy-only user workflow.

## Environment import: same preview as the model importer

Open **Terrain Tools > Customize Sky > Import Sky Style...** or **Terrain Tools > Customize Vegetation > Import Grass Style...**.

Pick an FBX, DBO or X model. PW Edition now opens the existing GameGuru model importer, which displays the textured 3D mesh and retains its existing material editing and adjustment tools. While the model importer is active, the persistent **Inspector** provides a style-name box, **Size (%)** slider and **Yaw** slider. Save using the importer’s existing Save action. For the sky/grass modes this exports a native style folder rather than an ordinary entitybank FPE. Invalid or duplicate style names and missing textures fail instead of overwriting another asset.

- **Sky:** native `Files/skybank/<name>/<name>.dbo`, `skyspec.txt`, relevant textures and `pw_style_settings.txt`. The existing sky renderer reads optional scale/yaw and otherwise uses its usual defaults.
- **Grass:** native `Files/vegbank/<name>/veg.dbo`, `grass.png` or `grass.dds` albedo atlas, relevant source textures and `pw_style_settings.txt`. The existing vegetation renderer reads optional scale/yaw with legacy fallback.

The existing importer is still responsible for mesh/texture validation and preview. This is not a general-purpose HDRI/cubemap converter and does not support arbitrary terrain shaders. FBX remains enabled. **GLB is disabled** in the file picker, legacy picker and importer entry gate; the experimental bridge source remains in the repository but is not a user-facing supported format.

## Multiple painted grass types

In **Terrain Tools > Customize Vegetation > Mix Multiple Grass Types**, choose up to **four** styles. Each row has:

- Enable/disable
- Style selector from the current vegetation bank
- **Spread weight (%)** from 0 to 100

The mix normalises the active weights. For example, 25 and 75 distribute painted grass into approximately one-quarter of the first style and three-quarters of the second, using deterministic seeded cell selection. Every mixed instance uses its chosen **actual vegetation mesh and atlas**, not a material colour blend. With zero or one active style, GameGuru's native single-style vegetation grid remains the renderer.

Because Classic's native vegetation grid supports only one mesh/atlas, PW Edition adds a *bounded* camera-centred object-instancing renderer for mixed grass. It samples the existing painted vegetation mask and limits visible vegetation to **320 mixed instances** within the nearby grid. This is an initial performance-protected path, **not** equivalent to the performance/culling/shadow behaviour of the original native renderer. It needs GPU profiling, correct transparency/wind tests, rotation/ground-contact review and standalone map testing. Grass density also depends on the painted mask and existing vegetation-quantity setting.

The choices are stored in the optional `pw_grassmix.cfg` entry inside the FPM map archive and restored when an FPM is loaded. Older FPMs without this entry keep legacy one-type grass. Importing custom source grass styles and distributing game asset bank dependencies with standalone exports require separate acceptance tests.

## Validation required

1. Full Windows x64 editor compile and relaunch; confirm both import buttons show the actual 3D preview and applied textures.
2. Import a non-empty FBX sky dome, change size/yaw, save, select and test it in editor and Test Level.
3. Import a textured FBX grass mesh (PNG or DDS), save to native vegetation style and paint it successfully.
4. Select two to four styles, change weights, test visibly different mesh distributions around painted regions; ensure non-painted regions stay bare.
5. Save/reopen an FPM and verify the same enabled styles, weights and object distribution.
6. Check RAM/VRAM, CPU frame time, transparency, fog, wind, LOD/shadows and project transitions with the 320-instance cap. The old/native single-style path must remain unaffected.
7. Confirm GLB no longer appears as an advertised option and direct GLB attempts give a clear unsupported message.

Until those tests pass, **do not distribute this as a verified installer**.
