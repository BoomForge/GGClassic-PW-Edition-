# The GameGuru Classic Repository

This is not open source and remains the copyright of The Game Creators Ltd, see Wiki for more information on license matters. This repository contains a 64-bit version of Classic as the master trunk, and an older version under the 'VS2017-Legacy-JAN2023' branch that used the older VS2017 for compiling.

GameGuru Classic is a 3D game maker for simple first and third person games and is available to buy from Steam. 

Steam Product: https://store.steampowered.com/app/266310/?utm_source=githubrepo&utm_campaign=general&utm_medium=web

You will need the media files associated with the above product to use this repository.

This source code requires Visual Studio 2022 Community Edition. You can get this for free from the Microsoft website.

When installing Visual Studio 2022, ensure you select the "Desktop C++" development track, and tick the optional Visual C++ MFC for x86 and x64 as you will need these additional files to compile the legacy GameGuru Classic executables.

To give you a better idea about getting started, a while ago we made a 12 minute video showing the whole setup process from the very first download to running GameGuru Classic under your own binary creations. This video shows VS2017 being used but the process remains the same.

How To Use Repository Video: [[Click To Watch Video]](https://www.youtube.com/watch?v=gtkmwEDivxo)

## F.A.Q

Q> Is GameGuru Classic open source?
A> No, the product and it's source code is copyright The Game Creators Ltd. We make the source code available so contributors from the community can integrate additional functionality, and to enable any user to scan through the code to find out how the inner workings of this game maker works.

Q> I can no longer find the LFS large media files in the Repository?
A> Due to this public repository being hammered by anonymous cloning, we have moved the media files to "GAMEGURUREPO_LARGEFILES.zip", available from our GDrive at: https://drive.google.com/file/d/17nCbOLPEbfHLbgN8ua7CgJQJMWpELQGG/view?usp=sharing

---

## Pixelwolf Edition — Feature Upgrade Tracker

**Project:** [GGClassic PW Edition](https://github.com/BoomForge/GGClassic-PW-Edition-)  
**Development branch:** [work/pw-community-integration-20261121](https://github.com/BoomForge/GGClassic-PW-Edition-/tree/work/pw-community-integration-20261121)  
**Review:** [Draft PR #1](https://github.com/BoomForge/GGClassic-PW-Edition-/pull/1)  
**Last updated:** 21 November 2026

The goal is a **fast, familiar first-person world builder** for an early-2000s-style 3D RPG that will grow gradually over a long period. DDS support must remain intact. New graphics formats are **additional**, not replacements.

Statuses below distinguish **committed source changes** from **tested runtime behaviour**. A checked source task does not mean a Windows executable has been built or verified.

### Completed — checked and committed in source

- [x] Forked the latest official **64-bit GameGuru Classic** source. The initial fork matched upstream commit `821f688` exactly.
- [x] Audited community forks: the investigated Preben/plemsoft and Cybernescence performance improvements were **already included upstream**, so no older engine changes were blindly cherry-picked.
- [x] Staged the community animated alpha/emissive PBR shader (upstream [PR #6261](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/pull/6261)); **runtime test outstanding**.
- [x] Added a **DDS-first PNG fallback** to entity texture lookup in `M-Entity.cpp`, including diffuse/base, normal, specular/metalness, gloss, AO, height, illumination/emissive/detail, and supported multi-material override paths. Existing explicit `.png` paths continue to load normally; **runtime test outstanding**.
- [x] Extended `M-Importer.cpp` to find same-name `.png` files when a converted FBX's referenced texture cannot be located, while continuing to prefer an available `.dds`.
- [x] Extended `M-MapFile.cpp` to preserve PNG `_D`, `_N`, `_S` companion files in the existing level/map archive workflow without removing DDS handling.
- [x] Created a detailed [technical roadmap](docs/PIXELWOLF_CLASSIC_ROADMAP.md) of remaining engineering risks, source references and test criteria.

### In progress — verification needed

- [ ] **PNG + DDS regression test on Windows:** build and load equivalent sample entities using `_D.dds` and `_D.png`; confirm diffuse, normal, metalness/specular, AO, gloss, height, emissive, transparency, material overrides, save/reopen, map archives and standalone exports.
- [ ] **Shader validation:** compile and run the community animated alpha/emissive PBR shader; compare unchanged default shaders for regressions.
- [ ] **Build and stability baseline:** compile the fork using Visual Studio 2022, Desktop C++, MFC and the original legally owned GameGuru media; test editor launch, tiny FPS level, play/stop, save/reopen and standalone game.
- [ ] **Loading-time investigation:** measure cold startup, first test, repeat test, return to editor, and standalone load for a tiny world. Do not claim speedups without before/after measurements.

### To do — future upgrades

- [ ] Reduce unnecessary editor-to-test **loading times** after profiling; preserve caches and correctness. [Upstream #3100](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/3100).
- [ ] Improve **Blender/Mixamo animated character FBX imports**, including skeletons, animation clips and texture paths. [Upstream #1894](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/1894).
- [ ] Fix **texture overrides, material paths and missing textures** during import and game export. [Upstream #4659](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/4659).
- [ ] Research **glTF/GLB import** for static and rigged assets, without removing legacy X/FBX.
- [ ] Improve **live local-light shadows** for torches/lamps while retaining existing sunlight shadows and good GTX 1660-class performance. [Upstream #555](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/555).
- [ ] Investigate and improve **terrain performance and 64-bit object culling**. [Upstream #6223](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6223), [#5643](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/5643).
- [ ] Investigate **dynamic-light normal-map behaviour**. [Upstream #6209](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6209).
- [ ] Investigate **waypoint-editing stability** for later NPC work. [Upstream #6444](https://github.com/Dark-Basic-Software-Limited/GameGuruRepo/issues/6444).
- [ ] Confirm reliable Windows builds and regression checks before merging experimental engine changes to `master`.

**No Windows binary or performance improvement has been verified yet.** The first priority is maintaining a pleasant, fast world-building workflow—not building the entire RPG feature set now.

