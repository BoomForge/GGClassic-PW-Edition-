# Pixelwolf GG Classic — traditional game engine workspace

**21 Nov 2026 — source integrated on draft development branch; Windows gameplay/UI acceptance pending.**

The existing GameGuru Classic 64-bit ImGui editor already has a dockable asset/entity browser and editable entity properties. This update rearranges the **actual existing windows** rather than replacing game editing with an independent UI mock-up.

## Default layout

- **Left** — `Scene Hierarchy`: list of placed level instances with a search field, display names, locked marker and selected-item highlight. Clicking an unlocked instance requests selection through Classic's existing `iWidgetSelection` path so the viewport selection/gizmo is connected. The hierarchy never confuses the asset bank with placed instances.
- **Centre** — current editor/3D viewport, retaining existing scene controls.
- **Right** — a new persistent `Inspector` tab that displays selected object name, position, rotation and scale plus an `Edit Entity Properties` button. It delegates edits to Classic's existing full `Entity Properties` panel rather than bypassing the legacy editing and EBE lifecycle. The existing `Character Creator`, `Structure Properties`, `Importer`, terrain tools and waypoint windows also remain dockable on the right.
- **Bottom** — Classic's existing `TABENTITYNAME` asset/entity browser with all its original add/import/thumbnail behavior, docked along the lower workspace. This is not a new filesystem browser or a new format importer.
- Existing tutorial/help windows are tabbed into the right dock instead of shrinking the viewport into extra columns.

The dockspace identity has been versioned as `PWEngineDockspaceV1`, so upgrading from a previous PW or Classic `MyDockspace` layout creates this workspace once. Subsequently, ImGui saves/restores each user's adjustments through Classic's existing `uiv3.layout` mechanism when `pref.save_layout` is enabled.

To restore the arrangement manually use **Reset Layout > PW Engine Layout (Scene / Assets / Inspector)**.

## Selection safeguards

Hierarchy rows are sourced from `t.entityelement[1..g.entityelementlist]` and ignore unused slots. Locked instances are labelled and do not issue selection requests. Selection clears a previous gizmo selection and then reuses Classic's own `iWidgetSelection` routing rather than editing the entity array directly. Because there is no 3D raycast hit point when clicking a 2D scene tree, hierarchy selection uses zero pick offsets; an old viewport mouse hit is never reused to offset the selected object.

This is a **flat hierarchy** of placed objects for the first iteration, not yet a parenting/reparenting system. Existing transforms/materials stay in Classic's properties workflow; there is no rewritten component framework.

## Windows test checklist

1. Open the editor at 1280x800 and wide-monitor sizes; check that the viewport remains usable and the asset browser is along the bottom.
2. Save/restart after dragging dock splits; check persistence. Use Reset Layout > PW Engine Layout, verify the default returns.
3. Place two copies of an entity with different names; verify the hierarchy shows two *instances* instead of one bank entry and the persistent inspector shows the selected instance's transform.
4. Select an entity in the hierarchy, verify the 3D widget and persistent inspector respond. Click **Edit Entity Properties** and ensure Classic's existing full editor can modify/save the entity without losing it. Then switch between entities without incorrect gizmo jumps.
5. Lock an entity, confirm it is listed but cannot be selected in the hierarchy.
6. Search for one name; clear search; switch levels; confirm entries follow the active level without stale references.
7. Import FBX, save/reopen a map, test terrain/waypoint/structure modes, test standalone builds. The bottom browser must preserve all existing asset add/import operations.
8. Test resizing down to 1024x768 and capture screenshots for usability review.

**No claims of completed editor runtime testing until someone launches this build.**
