# Native third-person gameplay — owner priority

## Goal
A proper engine-supported third-person mode, not a gameplay script attached to an entity. Preserve existing first-person projects and existing Classic third-person marker compatibility.

## First source stage
The existing `tpp/thirdperson.lua` file is an empty marker. The real legacy camera/facing code is in `gameplayercontrol.lua`. The new native binding replaces that section when a valid non-VR single-player third-person avatar is active. C++ calculates orbit, shoulder offset, facing, nearest collision restriction and frame-time smoothing. The protagonist uses the existing physics body rather than a new duplicate player. Legacy fallback remains for incompatible binaries and `GGCPW_LEGACY_THIRDPERSON=1`. The modified stock controller must be copied and exported with the engine (build copy and existing export collection). Full engine build and actual runtime checks are pending.

## Remaining implementation order
1. Trace native physics movement and animation state contracts; extract third-person-specific movement and animation orchestration from Lua without changing first-person.
2. Native camera-relative movement, turning, free orbit, aiming/strafing, sprint/crouch/jump/fall/land, slope and wall collisions; configurable camera sensitivity, distance, shoulder and target framing. Camera override and indoor clearance tests.
3. Native combat: camera aim target with muzzle/hand obstruction, melee, ranged projectiles, hit attribution, third-person weapon attachments and animation transitions. No shooting through walls.
4. Explicit editor player mode and protagonist model selection, rig/animation mapping and preview; integrate existing player settings and importer/character creator roadmap. No required entity gameplay script.
5. Player death/ragdoll/respawn, checkpoints, teleports, level transitions and persistent profiles; character and weapon state restoration.
6. Acceptance: actual editor and exported Windows game with a licensed character, first-person regression, low/high FPS, walls and corners, stairs, terrain, aim, melee, ranged, all locomotion states, custom model, death/respawn and save/reload. Validate update packaging and do not mark full native support complete until owner testing.

Current limitations: Lua still owns animation, movement and combat policy, camera callbacks rely on updated shared controller, static clearance rays are not a true sphere cast and dynamic obstacles/aim alignment need further work. No new model or proprietary media added.
