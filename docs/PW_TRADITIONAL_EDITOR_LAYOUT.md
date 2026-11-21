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

## Owner runtime feedback and corrective UI pass — 21 November 2026

The first PW Edition build started and its in-app updater works, but owner testing failed usability acceptance: text-only icons, floating entity library, overlapping independently moved panes, a >60-second test-project load, and absent visible real-time shadows.

Corrective changes on development branch: PWEngineDockspaceV2 docks the full Entity Library and Current Objects along the bottom, with scene hierarchy left and inspector right; core workspace windows use NoMove with split-resize still available. New update-generated icons are locally drawn visual pictograms, not filename tiles; old PW-generated text placeholders are recognized for replacement while non-matching stock/custom icons are preserved.

The PW Edition menu now offers Capture Load Diagnostics (CSV) and Open Load Diagnostics Folder. Begin-stage markers survive a stall and slow-stage markers flag completed operations >=10 seconds. The real-time editor shadows toggle defaults on across zoom levels; it can be disabled for performance.

None of these UI, loading, or shadow fixes is runtime accepted until tested on the owner's Steam install. Do not claim that the 60+ second load is fixed until a trace identifies the bottleneck and a before/after test confirms improvement.

## Original Classic artwork and integrated workspace repair (21 November 2026)

Owner's second runtime screenshot shows PW blue/cyan circular toolbar pictograms and repeated generic thumbnail images in the full bottom Entity Library. The requested visual design is the **original GameGuru Classic toolbar artwork**, with the newer traditional workspace geometry preserved.

The V4 ImGui workspace now splits hierarchy left and inspector right FIRST, then splits the remaining centre frame downward for the bottom Entities / Asset Library. The browser therefore shares splitters with the viewport, rather than spanning the whole desktop as a separate root-bottom strip. The bottom dock ID is recovered from saved ImGui layouts so opening the library later stays in the same frame.

The updater reads the original 42x42 toolbar strips from the licensed local GameGuru.exe using Windows RT_BITMAP resource IDs 152, 178, 180, 182 and 186 as recorded in GameGuru IDE/Editor.rc. It converts the matching strips into local ImGui PNGs for New/Open/Save, entity/marker/builder, seven terrain tools, waypoint controls and test/multiplayer (where present); the original GameGuru executable is only read, never replaced or redistributed. The updater recognises both generations of PW-generated placeholders and replaces them, while preserving actual stock/custom PNG files. Icons with no matching Classic resource have neutral fallbacks; **do not call those original art**.

The asset browser now checks the actual per-entity preview path with the Windows filesystem, looking for BMP, PNG, JPG and JPEG instead of trusting the older Dark Basic FileExist check alone. If no source thumbnail exists, the first five missing names are logged; it does not invent thumbnail artwork or overwrite purchased asset folders.

Acceptance not established: owner must verify original resource extraction count in updater log, inspect the toolbar, test preview images in an asset pack known to contain thumbnails, resize all three shared splitters, save/reopen a map, and verify in-app updates. Windows CI compilation alone does not prove source bitmaps exist in this owner's Steam version.

## Runtime regression from owner's screenshots — corrective update (21 November 2026)

The owner's 21 November Windows screenshots **disproved** the prior claim that the bottom asset browser and Classic appearance had been fully restored. The dock displayed the current-level `TABENTITYNAME` list (empty in an untitled map), while the actual `Entity Library##ExternalWindow` asset/folder browser defaulted to closed. The blue/cyan circular toolbar icons were local PW-generated replacements. An alpha-blended library surface could show rendered terrain behind the panel.

**Implemented in development source, awaiting Windows acceptance:**

- `PWEngineDockspaceV5`: initialise the actual `Entity Library##ExternalWindow` as open, dock it first in the lower central frame, keep the current-level entity list as a secondary tab, and rebuild the default layout once. Use the original editor's neutral `myStyle2` (not the newer blue theme), applying the style preference before calculating its colours.
- Force the docked asset-library window's background opaque so viewport imagery does not bleed through; preserve the hierarchy and inspector side docks.
- Stop the independently authored updater from generating blue/cyan pictogram PNGs. Recover unmodified, identifiable originals from the owner's verified `PW-Edition-Backups` folders where available, then attempt extraction from the locally licensed original `GameGuru.exe` toolbar bitmap resources; preserve other existing art. **Not every missing Classic icon is guaranteed recoverable.**
- Asset previews continue to probe their own BMP/PNG/JPG/JPEG files. Missing/failed images no longer silently use `TOOL_ENTITY` (the circular toolbar asset image). The grid shows a clickable **No preview** placeholder for unrecoverable items instead.
- The owner's existing inventory includes real FPE/BMP pairs; the earlier completely empty panel is therefore not evidence that their GameGuru assets were absent.

**Acceptance still required** on the owner's Windows machine: full source build, `Entity Library` visible on launch, folder sections populated, at least three distinct real asset thumbnails loading, lower/side panel splitters resizing without overlap, original icons recovered and reported accurately in updater log, and neutral Classic style. Keep PR #1 in draft until accepted. Do not call a successful CI compile a visual test.

## V6 corrective implementation — duplicate asset panels and authentic icon verification (21 Nov 2026)

The owner's latest screenshot still showed an overlapping `Entity Library` AND `Entities` panel with a transparent background, circular PW-generated graphics, and inaccurate thumbnails. Source has now been changed as follows:

- `PWEngineDockspaceV6`: one bottom-central `Asset Browser##PWAssetBrowser` window docked into the SAME left/centre/right workspace node tree; no `TABENTITYNAME` window or dummy `##Object Library ExternalWindow` is created. The previous `Entities` window lists entities already placed in a map, NOT the disk asset library. Scene Hierarchy provides level-instance selection.
- The Asset Browser is permanent, has no separate close button, tracks its actual dock node after saved-layout restore, and retains Classic's folder/category filters, asset thumbnails, and click-to-add behaviour. Its window background is opaque.
- Reset the colour palette to the original-style light Windows appearance through `myLightStyle`, not the modern dark/cyan PW colour scheme. Do **not** call this a pixel-identical restoration of the MFC launcher: the new editor uses ImGui.
- Stop loading the generic `Files/editors/gfx/missing.bmp` as if it were each asset's actual preview. Genuine BMP/PNG/JPG/JPEG files are loaded; absent previews use a labelled placeholder with the existing click-to-add operation.
- The local Windows updater now detects antialiased blue-circle generated icons by scanning the known cyan rim, tries verified owner's backed-up originals and local Steam GameGuru.exe resources, and logs an explicit list of still-unrecoverable original toolbar images. **It cannot manufacture missing copyrighted stock artwork.**

**Acceptance:** Build success only verifies compilation. The owner must run the actual Windows editor and confirm one docked bottom browser, visible grouped folders, diverse correct previews, no viewport bleed-through, Classic-style palette, and original icon restoration. The draft PR remains unmerged and any items not visually verified remain outstanding.
