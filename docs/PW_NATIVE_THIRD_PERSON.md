# Native third-person gameplay — owner priority

## Goal
A proper engine-supported third-person mode, not a gameplay script attached to an entity. Preserve existing first-person projects and existing Classic third-person marker compatibility.

## First source stage
The existing `tpp/thirdperson.lua` file is an empty marker. The real legacy camera/facing code is in `gameplayercontrol.lua`. The new native binding replaces that section when a valid non-VR single-player third-person avatar is active. C++ calculates orbit, shoulder offset, facing, nearest collision restriction and frame-time smoothing. The protagonist uses the existing physics body rather than a new duplicate player. Legacy fallback remains for incompatible binaries and `GGCPW_LEGACY_THIRDPERSON=1`. The modified stock controller must be copied and exported with the engine (build copy and existing export collection). Full engine build and actual runtime checks are pending.

## Owner clarification — combat and interaction are the acceptance gate (25 November 2026)

The owner recalls the existing mode being limited by lack of usable combat and interaction. A prettier camera is not sufficient. Prioritise these gameplay capabilities alongside the minimum movement/animation foundation, ahead of cosmetic camera polish.

Confirmed code findings (not yet reproduced in Windows gameplay):
- `G-Gun.cpp::gun_change` explicitly returns for an already-equipped third-person character ("cannot equip new weapons for the moment"). Weapon pickup/equip/unequip/switch, ammunition, inventory and attachment lifecycle must be integrated rather than merely removing the return.
- Existing melee uses horizontal range, height and facing checks but no obstruction test in the strike loop. It also calculates `rand()%iDamageRange`, which is invalid when minimum and maximum damage are equal. Fix damage range, target eligibility, obstruction, animation strike windows and one hit per target per swing. Add unarmed, sword and other melee tests.
- Existing ranged code uses a special camera ray (unlocked view), a hard-coded short range for locked view, and a separate third-person projectile attachment path. Design a coherent aim-target-to-muzzle trajectory with obstruction checks, configured range, aim alignment, hit attribution and reticle behaviour.
- `M-LUA-Entity.cpp::entity_lua_getentityplrvisible` temporarily hides the protagonist to avoid self-intersection in door scripts. Replace fragile self-hit handling with an explicit player-body/interaction origin and exclusion policy; keep true camera queries separate. Validate range, visibility, occlusion, nearest target and action input, including doors, switches, pickups, NPC conversations, chests, quest objects, triggers and interaction prompts.
- Preserve existing first-person APIs and legacy Lua scripts where possible. Provide a semantic interaction API and make the remappable interact action usable from both modes.

**Acceptance gate:** one small playable test level with a third-person character able to walk/run, interact with doors, talk to an NPC, collect items, equip/switch weapons, attack a damageable enemy with melee and ranged weapons, reload, take damage, die/respawn and save/load. Obstructed interactions and attacks must not pass through walls, and the camera's position must not determine gameplay reach. Test both first-person and third-person modes on Windows. Source audit is not a claim these repairs are already implemented.

## Remaining implementation order
1. Trace native physics movement and animation state contracts; extract third-person-specific movement and animation orchestration from Lua without changing first-person.
2. Native camera-relative movement, turning, free orbit, aiming/strafing, sprint/crouch/jump/fall/land, slope and wall collisions; configurable camera sensitivity, distance, shoulder and target framing. Camera override and indoor clearance tests.
3. Combat and interaction (highest gameplay priority, develop in parallel with the minimum movement/animation foundation): camera aim target with muzzle/hand obstruction, melee, ranged projectiles, hit attribution, equip/switch and weapon attachments; shared interaction origin, semantic action, line of sight, target selection and stock door/pickup/NPC compatibility. No shooting or interacting through walls.
4. Explicit editor player mode and protagonist model selection, rig/animation mapping and preview; integrate existing player settings and importer/character creator roadmap. No required entity gameplay script.
5. Player death/ragdoll/respawn, checkpoints, teleports, level transitions and persistent profiles; character and weapon state restoration.
6. Acceptance: actual editor and exported Windows game with a licensed character, first-person regression, low/high FPS, walls and corners, stairs, terrain, aim, melee, ranged, all locomotion states, custom model, death/respawn and save/reload. Validate update packaging and do not mark full native support complete until owner testing.

Current limitations: Lua still owns animation, movement and combat policy, camera callbacks rely on updated shared controller, static clearance rays are not a true sphere cast and dynamic obstacles/aim alignment need further work. No new model or proprietary media added.
