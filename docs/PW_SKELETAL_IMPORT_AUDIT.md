# PW Edition: first native skeletal importer upgrade

**Implemented in source — 21 November 2026.** Native GameGuru inspector reads actual frames, skin bone names and counts, animation sets, tracks and key counts. A read-only Skeleton & Animation section in the existing importer previews recognized Mixamo-to-Bip01 bone names and calls out unmapped or ambiguous names.

**Optional save verification:** Set environment variable GGCPW_VERIFY_SKELETON_SAVE=1 before launching the editor. When saving an animated/skinned DBO through either existing native save path, the importer independently reloads a scratch object and checks for reduced skin bone references, missing bone names or missing animation tracks/keys. Warnings go to OutputDebugStringA and a popup. This is off by default and never changes source animations.

**Tests:** tools/test_pw_skeletal_audit.cpp plus Windows CI compilation exercise bone-name mapping, inspection, and simulated data loss.

**Limits:** Does not perform new retargeting, compare bind matrices or vertex weight values, validate runtime animation playback, or rebuild Character Creator. The existing legacy Uber animation option still overwrites source animation data when explicitly used. External Three.js sources from the recovered Model Bridge were not copied into this engine.

**Verification still required:** Windows engine linking is currently blocked by missing DirectXTex.lib on CI. User-machine testing must cover static FBX, Mixamo FBX, animated X, exported DBO reload, game preview and standalone game animation.
