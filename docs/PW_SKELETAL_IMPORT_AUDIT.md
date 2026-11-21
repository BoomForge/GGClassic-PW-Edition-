# PW Edition — native skeleton preservation validation (M3.1)

**Status: implemented in source 21 November 2026. Standalone Windows tests pass; full engine build/runtime tests remain a separate gate.**

## Implemented code

1. PW-SkeletalAudit.h inspects native GameGuru object frames, skinned mesh bone references and animation track/key counts. It previews non-destructive Mixamo-to-Bip01 name mapping.
2. PW-SkeletalIntegrity.h reads the engine's actual sBone vertex indices, per-vertex influence weights, bone translation/bind matrices, sFrame original matrices and per-key animation data. Float fingerprints are quantized to 1e-4 to avoid raw padding/float-byte comparisons. It checks vertex-index ranges, invalid weights, per-vertex normalization, missing buffers and enormous arrays.
3. The ImGui importer exposes an explicit "Scan skin weights and animation clips" button, showing source clip names, lengths in engine ticks, track counts and key counts. The full scan is manual so it cannot stall every editor frame.
4. GGCPW_VERIFY_SKELETON_SAVE=1 enables a DBO save/reload audit for both existing importer save variants and the existing Character Creator save. It loads the saved DBO in a scratch object, compares counts, names, fingerprints of skin weights, bind matrices and animation key data, reports warnings via debugger/popup, and removes the scratch object. Without that setting, the normal exporter is untouched and the cost is avoided.
5. Windows MSVC test_pw_skeletal_audit.cpp and test_pw_skeletal_integrity.cpp validate the core logic. The latter intentionally changes vertex weights, bind matrices, frame transforms, clip lengths and keyframe values while preserving counts. Simulated missing buffers and invalid indices are also tested.
6. The full Windows CI build now compiles vendored DirectXTex.lib and Effects11.lib from source before attempting to link the engine; both SDK dependency steps have passed on CI.

## Limits / safeguards

- **Audit does not repair or retarget data.** It detects alterations caused by saving where inspectable. It does not yet provide full end-to-end skinned FBX/GLB import, native playback proof, bone orientation retargeting or modular character building.
- This first fingerprint is order-sensitive within a mesh's influence list and a clip's animation tracks. Reordering logically equivalent data can generate a warning. A true semantic equivalence comparator can be added if that occurs in real-world exports.
- The scan checks exported weights, matrices and logical key payloads, not interpolation caches, shader skinning on the GPU, animations appearing correctly in a game, or test-level/standalone gameplay.
- The old "Use Uber Animations" option is still the legacy, destructive source-clip replacement path when a user explicitly selects it. No new code activates it.
- The importer ImGui window is compile-time guarded by ENABLEIMGUI and has an early-return path without VRTECH; **do not assume the new panel is visible in every Classic build** until checked in a runnable editor. Save-time verification is not dependent on that UI path.
- 1e-4 quantized payload comparison can flag harmless rounding at a bucket boundary, and is not proof of bit-for-bit serialization.
- Do not publish a GameGuru-derived executable without verifying rights. Source modifications are on a development branch, not deployed to the Steam installation.

## Next engineering gate

- Complete full engine linking and run static FBX, Mixamo FBX, DirectX X and existing animated DBO tests on Windows with export/reimport.
- Verify actual skin deformation, rest pose and playable clips in an exported game.
- Build a versioned non-destructive retarget profile (named bones, bind-space orientation, root motion and clip conventions) and preserve the original animation as a separate source before applying conversion.
- Move character creator to the same reusable versioned rig contract only after those tests.
