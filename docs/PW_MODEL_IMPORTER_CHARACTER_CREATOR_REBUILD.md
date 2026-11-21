# PW Edition — Model Importer and Character Creator rebuild

**Decision date:** 21 November 2026. **Status:** staged implementation begun. Native skeleton/animation export auditing is in source; replacement importer, actual retargeting and Character Creator 2 authoring UI remain unimplemented. Legacy tools are retained.

## Recovered Model Bridge v1.0.7 source — VERIFIED 21 November 2026

The project owner supplied `EFPSE-Model-Bridge-v1.0.7.exe` (SHA-256 `73bfb2f5c4f2fddc316957abc1818d4acb964eee6a3e73e35401546000bfdf46`). Offline inspection identifies a **Go 1.23.2 Windows x64 binary** with a `go:embed` browser application. We reconstructed its embed.FS name/data pointer table, preserving original file bytes and lengths. **33 files (1,285,624 bytes) were extracted. All 30 embedded JavaScript files passed syntax-only checks.** No EXE or recovered JS was executed.

Recovered source includes:
- `web/app.js`: native frontend workflow for OBJ/FBX/GLTF/GLB/DirectX X; file and texture discovery, animation mixer/clip preview, attachments to bones, animation pose sampling, material assignment, thumbnail and export assembly.
- `web/x-parser.js`: DirectX .x mesh parser with SkinWeights, AnimationSet/AnimationKey and animation tick metadata.
- `web/x-loader.js`: DirectX skeleton hierarchy, inverse bind poses, up-to-four normalized skin weights, skinned mesh and clips. This is strong reference code for a **skeleton-preserving GameGuru loader**.
- `web/ascii-fbx.js`: static geometry-only ASCII FBX fallback. The **animated** FBX path uses bundled Three.js loader, not this parser.
- `web/vendor/local-three.js` + `web/vendor/three-runtime.js`: vendor loader/render/animation runtime; audit upstream licences before copying to any distributable engine.
- `web/conversion-utils.js`, `web/material-texture-utils.js`, `web/alpha-material-utils.js`, `web/gameguru-fpe.js`, `web/sampling-utils.js`, `web/md3.js`: coordinate and scale, material/alpha/FPE metadata, time sampling and MD3 generation.
- Original `web/*test.mjs` regression sources, all syntax checked (NOT executed).
- v1.0.7 explicitly **disables Rhino 3DM import** despite earlier advertised support.

**Crucial boundary:** Model Bridge **does load skeletal source data**, and plays source clips, but **bakes skinned vertex positions to MD3**, losing bones and per-vertex skin weights in final output. No generic Mixamo-to-GameGuru animation retargeter was found. We must implement and validate an explicit native bone mapping / skeleton preserving export contract.

**Remaining unrecovered component:** the Go HTTP/backend source. The browser frontend calls `/api/convert-dds`, `/api/decompress-x`, `/api/export-asset`, `/api/export-package`, `/api/project-info`, `/api/select-project`, `/api/session`, `/api/ping`. These remain compiled Go implementations, and cannot be claimed as original source. The older v1.0.4 Source.zip is still not located. EFPSE Studio bundled v1.0.4; version compatibility with 1.0.7 is not yet proven.

Recovered source and SHA manifests were packaged as a downloadable ZIP in the owner's ChatGPT development conversation. **Not automatically checked into this GitHub repository** pending a licence/dependency audit and conscious selection of code to port. Re-run an extraction from the original user-supplied v1.0.7 binary when preparing a port.

## Model Bridge source discovery

Searched all 30 accessible BoomForge GitHub repositories (including archived repos), repository names, public search and related commit history. No standalone EFPSE Model Bridge converter source repository was found.

- **Verified EFPSE Studio build integration:** BoomForge/EFPSE---PW's installer/main.go uses `//go:embed assets`, checks the SHA-256 of `EFPSE-Model-Bridge-v1.0.4.exe` (`62a5f26a87ad2c334a0b17c44fc83045cf3ef175882eb789b8c37cc4ef196f66`), and installs that executable under `ModelBridge/`. Its `EFPSEBridgeIntegration.cs` launches the process, while `PixelwolfModelImportReview.cs` validates/reviews the resulting MD3/PNG assets. Thus EFPSE Studio **bundles and uses the working converter** but its GitHub snapshot does **not contain the converter's importer/animation source**. README explicitly lists the EXE among large binary dependencies excluded from GitHub.
- Commit 207ff498 in EFPSE---PW verifies that the external Model Bridge responds to EFPSE_MODEL_BRIDGE_INITIAL_PROJECT. It does not implement FBX loader or retargeting.
- Archived EFPSE Model Bridge documentation describes Three.js FBXLoader/GLTFLoader, skeletal/morph clip preview, and baking vertices to Quake III MD3 frames. **MD3 vertex frames do not preserve bones and weights**, so the bridge is not an already-working Mixamo/GameGuru retargeter.
- A ChatGPT Library checksum manifest mentions EFPSE-Model-Bridge-v1.0.4-Source.zip. The ZIP itself has not been recovered/inspected. Its checksum is not source code.
- BoomForge/Modeling-Interface-for-Low-poly-Objects has a separate retro-character-creator-foundation branch with Blender/Python character creation modules, rig/morph/clothing design and export tests. This can inform GameGuru design, not be directly copied as C++ code.

**Source recovery gate:** Locate an actual source archive (or external original repository), compare its hashes and dependencies, confirm redistribution licences, and inspect animation-sampling and material-conversion internals. Do not infer the implementation from the wrapper.

## Rebuild intent

Create two cohesive GameGuru Classic tools that share a single asset/material/rig/animation pipeline while remaining compatible with original X/FBX/DBO GameGuru assets and FPE entities:

### Model Importer 2

M1 — **Inspect before import:** OBJ, FBX, legacy GameGuru formats; GLB/glTF after library/format evaluation. Display hierarchy, materials, texture paths, UVs, mesh counts, units/axes, bones, skin weights, bind pose and animation clips. Warn about unsupported/missing data. Asset staging should not overwrite existing game objects.

M2 — **Preview and edit:** orbit/pan/zoom, reference-scale grid, transform orientation, pivot, material and texture assignment, alpha/emissive maps, DDS-first PNG/JPEG/TGA companion fallback, collision shape and export paths. Preserve multi-material assignments and UVs.

M3 — **Skeleton and animation:** preserve original skeleton, bind pose, vertex weights and clip ranges; interactive bone mapping and clip preview. Add an explicit, versioned GameGuru/Pixelwolf humanoid retarget profile for Mixamo and Blender. Correct axes, bind poses, scale, root motion and frames. Do not pretend arbitrary skeletons retarget automatically. Offer baked animation only as a clearly labeled fallback with limitations.

M4 — **Safe native export:** stage generated DBO/FPE/textures in a temporary directory, validate, back up existing files before overwrite and commit only after successful checks. End-to-end test Blender static FBX, mixed materials, Mixamo rigs, missing textures, long paths, map save/reopen, Test Level, standalone export and animation playback.

### Character Creator 2

C1 — **Canonical, versioned body + rig:** deliberately low-poly base topology, UVs, skin weights and a documented reusable humanoid skeleton. Audit legally distributable base assets. Use M.I.L.O retro character concepts as research; do not require its Blender UI at runtime.

C2 — **Practical creation UI:** choose a base, change stature/build/head/face/skin, add swappable hair/clothes/accessories, preview animation, save and reopen editable presets. Clothing uses anchored bones and optional region masks to reduce clipping. Keep comfortable PS1–PS2 era defaults.

C3 — **Shared animation and export:** reuse Importer 2's bones/retarget/clip preview and native GameGuru entity exporter. Characters remain editable via a separate preset, while game assets save to normal GameGuru-compatible FPE/DBO/texture files. Validate idle/walk/run/attack and any supported facial/eye systems.

## Engineering order and compatibility gates

0. Clear the existing full VS2022 x64 engine compilation failures and secure an Editor / Test Level / standalone baseline. Do not install into Steam without a separately tested overlay.
1. Recover/inspect actual Model Bridge source if available. Start using independent controlled FBX/GLB sample assets if the archive cannot be found.
2. Implement a shared asset import manifest and M1/M2 preview behind a switch; leave legacy importer intact.
3. Build M3 skeletal animation mapping/retarget only when native skeleton data round-trips successfully.
4. Finalize the canonical body/rig spec; adapt M.I.L.O character research, verify base asset licence.
5. Implement C2 body/face/clothing authoring and C3 animation/export using the shared pipeline.
6. Validate every milestone with save/reopen, level playback, standalone export, original asset compatibility and performance measurements. Replace legacy UI only after evidence of equivalent functionality.

No unverified claim of working Mixamo retargeting or a functioning new character creator is authorized by this planning document.

## 21 November milestone — shared native skeletal integrity audit (M3.1)

GameGuru's existing sBone/sFrame/sAnimationSet data is now inspected without modifying the loaded object. The new `PW-SkeletalIntegrity.h` compares affected vertex indices and weights, bone and frame rest matrices, and per-key animation payloads. The current importer offers an optional on-demand deep scan in its ImGui pathway, while `GGCPW_VERIFY_SKELETON_SAVE=1` enables separate scratch-object export/reimport validation in both importer variants and Character Creator Plus. Two isolated MSVC tests pass. The previous missing `DirectXTex.lib` and `Effects11.lib` build dependencies are now built by CI before linking. See `docs/PW_SKELETAL_IMPORT_AUDIT.md` for test scope, conditional UI limitations and remaining proof requirements. This is an audit milestone **not** working skeletal retargeting or a rebuilt character creator.


## 22 November milestone — Model Importer 2 M1 shared preflight

**Implementation:** `GameGuru Core/GameGuru/Include/PW-AssetManifest.h` introduces the versioned `pw.asset-preflight` JSON contract (v1). The live ImGui importer now has a **Model Importer 2 - Preflight** expandable section with an explicit **Inspect loaded model** action and **Copy preflight manifest (JSON)**. The inspector collects existing native mesh index, vertex counts, number of skin bones and material stages, frame-to-parent names, texture-stage references and their resolved source/converter paths, and source animation clip names/durations/track counts. Failed texture resolution and missing buffers produce warnings. Inspection has bounds for mesh/frame/texture/clip enumeration and does not mutate the engine's object, textures, skeleton or export path.

**Compatibility:** No replacement importer format, automatic retarget, auto-overwrite or proprietary Model Bridge code was introduced. Existing Classic and PW export workflows are unchanged. The JSON is for inspection and later sharing with Character Creator 2, not an executable conversion recipe, and uses explicit schema/version identifiers. The panel snapshots only when asked; inspect again after modifying or reloading the model.

**Automated checks:** `tools/test_pw_asset_manifest.cpp` covers DDS-first fallback from a stale artist path, converter-folder PNG discovery, relative/absolute texture paths, missing textures, mesh/frame relationships, clip metadata, JSON escaping and damage-safe null table handling. The Windows skeletal audit workflow compiles/runs this test alongside the existing skeletal audit/integrity checks; a full Windows x64 solution build is the separate integration gate. Record the actual CI result before calling either successful.

**Still missing in Model Importer 2:** Complete materials/UV and axes preflight, in-editor material reassignment, human-readable missing texture resolution/repair, collision/pivot tools integrated into the new workflow, safe transactional native DBO/FPE export and true Mixamo/Blender skeletal playback/retarget validation. Do not call Model Importer 2 complete until those stages are implemented and user-accepted. Character Creator 2 authoring remains to follow the stable shared contract.
