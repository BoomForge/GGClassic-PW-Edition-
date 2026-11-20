# GGClassic PW Edition — Community integration and remaining issues

**Reviewed:** 2026-11-21 (Australia/Hobart)  
**Owner:** BoomForge  
**Source:** https://github.com/Dark-Basic-Software-Limited/GameGuruRepo  
**Fork:** https://github.com/BoomForge/GGClassic-PW-Edition-  
**Upstream baseline:** `821f688580821af64c8371f2b1f6e760b1c68e4f` (2026-08-27)  
**Working branch:** `work/pw-community-integration-20261121`

## Goal

Keep the familiar GameGuru Classic first-person visual world-building workflow while preparing a low-overhead, extensible foundation for *Adventuring for Dummies*. Start with a very small early-2000s-style 3D world, **not** a complete RPG. Avoid heavy engine rewrites and protect editor/test-play usability.

## Community contributions reviewed

The fork began **identical to upstream master** (comparison returned 0 commits ahead/behind).

- `plemsoft:master`: 0 commits ahead; performance work already incorporated into upstream.
- `cybernescence:master`: 0 ahead; older contributions are included upstream.
- `KronosInfraction:master`: 0 ahead; older than upstream.
- `RednibCoding:master`: 0 ahead.
- `Teabone3:master`: 1 ahead, but an unmerged example/script addition, not a loading or importer fix.
- Upstream PR #6261 (Lafette shader, submitted for integration): **staged in this branch** as `apbr_animwithtran_emissive.fx`. Supports an animated PBR material variant with alpha and emissive textures. Based on existing `apbr_core.fx` flags. **Not yet compiled or tested in-game.**
- Upstream PRs #6351 (LUT zone example), #6344 (unskippable video zone), #6196/#6141/#6138 (Lua comments/doc additions): **not imported**. These are not fixes for our immediate priorities, and some have behaviour or documentation risks.

### Validation status

**Verified:** fork is identical to upstream before branch edits; upstream shaders use the `ILLUMINATIONMAP`, `WITHANIMATION` and `ALPHACLIP` macros; staged shader matches the upstream PR.

**Not verified:** Windows compilation, shader runtime compilation, editor startup, faster loading, modified asset import, real-time shadow quality, or project compatibility. Do not call this a working or improved binary until tested on Windows.

## Remaining issues in priority order

| Priority | Area | Issue / evidence | Acceptance goal |
| --- | --- | --- | --- |
| P0 | Long editor-to-test/load times | [Upstream #3100](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/3100) | Benchmark cold launch, editor-to-test and repeated test of an empty map and a small populated map. Profile before altering the loader. Shorten repeat tests without breaking media or standalones. |
| P0 | Rigged/animated FBX imports | [Upstream #1894](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/1894) | Import a known Mixamo FBX character with materials and animations, save/reopen, test animation and export standalone. |
| P0 | Imported textures/materials | [Upstream #4659](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/4659) | Consistent Blender asset import, material paths, texture overrides, save/reopen. |
| P0 | Build and regression baseline | [Upstream README](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/blob/master/README.md) | Use VS2022 Desktop C++ + MFC and required purchased-product media. Record baseline build and reproducible small-map smoke tests before riskier changes. |
| P1 | Modern GLB/glTF assets | New enhancement; no verified built-in GLB pipeline | Evaluate safe Assimp/conversion option, preserve skinning and animation; do not assume a converter alone solves GG character rigs. |
| P1 | Dynamic local-light shadows | [Upstream #555](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/555) | Verify existing real-time directional sunlight shadows. Prototype shadows cast by movable/local lights separately with performance limits. |
| P1 | Terrain performance | [Upstream #6223](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6223) | Profile before changing terrain renderer; retain early-2000s visual target and low-spec GPU performance. |
| P1 | 64-bit culling | [Upstream #5643](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/5643) | Verify actual 64-bit visibility/culling and compare GPU draw calls in a repeatable scene. |
| P1 | Incorrect dynamic lighting/normal maps | [Upstream #6209](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6209) | Local lights should preserve material normal detail under test conditions. |
| P1 | Waypoint editing regression | [Upstream #6444](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6444) | Waypoints can be added, moved, saved, and used by a test NPC. |
| P2 | Shadow and lighting quality | [Upstream #1192](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/1192) | Lighting settings predictable; acceptable defaults for PS2/early-2000s look. |
| P2 | Independent shader contribution | [Upstream PR #6261](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/pull/6261) | Compile shader, render animated transparent/emissive entity, ensure no regression on non-animated models. |

## Implementation rules

1. Protect the upstream-compatible `master` branch. Use separate branches and PRs for improvements.
2. No bulk cherry-picking from older forks: the performance contributions found are already on upstream `master`.
3. Do not change core loading or importers without instrumented before/after measurements and a reproducible sample asset/level.
4. Keep work cumulative, with per-feature smoke tests and clear compatibility notes.
5. Distinguish **source change**, **build success**, **runtime validation**, and **user acceptance**. They are not equivalent.
6. Never add downloaded proprietary GameGuru media to this public repository.

## Licence and build prerequisites

This code is **source available but not open source**. It remains copyright The Game Creators Ltd. Consult the official repository's licence terms before distributing modified binaries. Per the upstream README, compilation needs Visual Studio 2022 Community with Desktop C++ and optional MFC components, as well as the game media owned through the product. The large repo media is distributed separately by the publisher.
