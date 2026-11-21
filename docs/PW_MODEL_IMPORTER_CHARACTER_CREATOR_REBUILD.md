# PW Edition — Model Importer and Character Creator rebuild

**Decision date:** 21 November 2026. **Status:** approved direction, engineering design. Neither module has been rebuilt yet; existing tools remain intact pending compatibility testing.

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
