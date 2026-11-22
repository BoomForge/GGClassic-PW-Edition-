"""Static regression checks for PW Editor V7 (not a substitute for Windows UI testing)."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parent.parent
GRID = (ROOT / "GameGuru Core/GameGuru/Source/M-GridEdit.cpp").read_text(encoding="utf-8")
UPDATER = (ROOT / "tools/PWBootstrapper/Program.cs").read_text(encoding="utf-8")
IMPORTER = (ROOT / "GameGuru Core/GameGuru/Source/M-Importer.cpp").read_text(encoding="utf-8")


class PWEditorUiRegressions(unittest.TestCase):
    def test_one_bottom_asset_dock(self):
        self.assertIn('PWEngineDockspaceV7', GRID)
        first = GRID.index("//#### Default dockspace setup")
        last = GRID.index('// Fully nested scene tree;', first)
        docking = GRID[first:last]
        self.assertEqual(
            docking.count('DockBuilderDockWindow("Asset Browser##PWAssetBrowser", dock_id_bottom);'), 1)
        self.assertNotIn('DockBuilderDockWindow(TABENTITYNAME', docking)
        self.assertNotIn('DockBuilderDockWindow("Entity Library##ExternalWindow"', docking)
        self.assertIn('sceneDock->LocalFlags |= ImGuiDockNodeFlags_NoTabBar', docking)
        self.assertIn('assetDock->LocalFlags |= ImGuiDockNodeFlags_NoTabBar', docking)
        self.assertNotIn('g_pwBottomAssetDockId = ImGui::GetWindowDockID();', GRID)

    def test_no_legacy_current_level_entities_pane(self):
        self.assertIn("g_pwShowLegacyLevelEntityList = false;", GRID)
        self.assertIn("if (g_pwShowLegacyLevelEntityList && refresh_gui_docking > 0)", GRID)
        self.assertNotIn('ImGui::Begin("##Object Library ExternalWindow"', GRID)
        self.assertIn('ImGui::Begin("Asset Browser##PWAssetBrowser", NULL', GRID)
        self.assertIn('ImGui::SetNextWindowDockID(g_pwBottomAssetDockId, ImGuiCond_Always);', GRID)

    def test_classic_style_and_opaque_browser(self):
        self.assertIn('PWApplyClassicEditorStyle();', GRID)
        self.assertIn('const ImVec4 face = PWClassicSystemColour(COLOR_BTNFACE);', GRID)
        self.assertIn('style.WindowRounding = 0.0f;', GRID)
        self.assertIn('myLightStyle(NULL);', GRID)
        self.assertIn('ImGui::SetNextWindowBgAlpha(1.0f);', GRID)
        self.assertIn('Colors[ImGuiCol_WindowBg].w = 1.0f;', GRID)

    def test_real_asset_previews_not_generic_fallback(self):
        body = GRID[GRID.index("void process_entity_library(void)"):]
        self.assertIn('".bmp", ".png", ".jpg", ".jpeg"', body)
        self.assertIn('if (!sImgName.empty())', body)
        self.assertIn('No preview##PWAsset', body)
        self.assertNotIn('sImgName = (g.mysystem.root_s + "files', body)

    def test_hierarchy_and_lua_integration(self):
        hierarchy = (ROOT / "GameGuru Core/GameGuru/Source/PW-SceneHierarchy.h").read_text(encoding="utf-8")
        lua = (ROOT / "GameGuru Core/GameGuru/Include/PW-LuaStudio.h").read_text(encoding="utf-8")
        self.assertIn('#include "PW-SceneHierarchy.h"', GRID)
        self.assertIn("PWRenderSceneHierarchy();", GRID)
        self.assertIn("iHasParentIndex = parent;", hierarchy)
        self.assertIn('PW_SCENE_FOLDER', hierarchy)
        self.assertIn('PW_SCENE_ENTITY', hierarchy)
        self.assertIn("PWSaveSceneFolders()", hierarchy)
        self.assertIn("editor_rec_addchildrentorubberband(e);", hierarchy)
        self.assertIn('Edit Lua Script...', GRID)
        self.assertIn('Edit Script##PWLuaMainAI', GRID)
        self.assertIn('PWLuaStudio::OpenEntityScript', GRID)
        self.assertIn('inline bool OpenEntityScript(HWND owner', lua)
        self.assertIn('if (!CanDiscard()) return false;', lua)

    def test_drag_path_bypasses_legacy_external_sync(self):
        fn = GRID.split('void AddPayLoad(ImGuiPayload* payload, bool addtocursor)')[1].split('bool TutorialNextAction')[0]
        self.assertIn('GetTickCount() - pwAssetStart', fn)
        self.assertIn('#if !defined(ENABLEIMGUI) || defined(USEOLDIDE)', fn)

    def test_permanent_asset_browser_and_fast_ui_insertions(self):
        # The library is a docked editor pane, never a transient popup.
        library = GRID[GRID.rindex("void process_entity_library(void)"):]
        self.assertIn("bExternal_Entities_Window = true;", library)
        self.assertNotIn("bExternal_Entities_Window = false;", library)
        self.assertNotIn("textureId = TOOL_ENTITY;", library)
        self.assertIn("int max_load_persync = 24;", library)
        self.assertNotIn("editor_filllibrary();", library)
        self.assertIn('textureId = 0; // No thumbnail:', library)

    def test_hierarchy_parent_and_lua_entrypoints_survive(self):
        hierarchy = (ROOT / "GameGuru Core/GameGuru/Source/PW-SceneHierarchy.h").read_text(encoding="utf-8")
        self.assertIn("PWSetSceneParent(int child, int parent)", hierarchy)
        self.assertIn("PWMoveEntityToFolder(int e, int folder)", hierarchy)
        self.assertIn("editor_rec_addchildrentorubberband(e);", hierarchy)
        self.assertIn("OpenEntityScript(GetActiveWindow()", hierarchy)
        self.assertIn('if (ImGui::Button("Edit Lua Script..."))', GRID)

    def test_importer_thumbnail_uses_camera_viewport_not_desktop(self):
        # A reported Serine import saved a solid-green BMP even though the
        # DBO held native geometry. Match GameGuru's working scene preview.
        capture = IMPORTER.split("void import_generate_thumb(void)", 1)[1].split(
            "#else\nvoid importer_save_entity", 1)[0]
        self.assertIn("FastSync();", capture)
        self.assertIn("m_mycam->viewPort3D.Width", capture)
        self.assertIn("m_mycam->viewPort3D.Height", capture)
        self.assertIn("ShowObject(t.importer.objectnumber);", capture)
        self.assertNotIn("GetChildWindowWidth(-1)", capture)

    def test_environment_imports_and_multi_grass_have_real_paths(self):
        terrain = (ROOT / "GameGuru Core/GameGuru/Source/M-Terrain.cpp").read_text(encoding="utf-8")
        veg = (ROOT / "GameGuru Core/GameGuru/Source/M-Grass.cpp").read_text(encoding="utf-8")
        maps = (ROOT / "GameGuru Core/GameGuru/Source/M-MapFile.cpp").read_text(encoding="utf-8")
        env = (ROOT / "GameGuru Core/GameGuru/Include/PW-TerrainStyleUI.h").read_text(encoding="utf-8")
        clone = (ROOT / "GameGuru Core/GameGuru/Include/PW-TextureStyleClone.h").read_text(encoding="utf-8")
        sky = (ROOT / "GameGuru Core/GameGuru/Source/M-Sky.cpp").read_text(encoding="utf-8")
        self.assertIn('Mix Multiple Grass Types', terrain)
        self.assertIn('Spread weight (%)', terrain)
        self.assertIn('PWGrassMix::Refresh(', veg)
        self.assertIn('InstanceObject(id, rs.source);', veg)
        self.assertIn('ReadMemblockByte(memblockId, offset)', veg)
        self.assertIn('PWGrassMix::Save(', maps)
        self.assertIn('PWGrassMix::Load(', maps)
        self.assertIn('AddFileToBlock(1, "pw_grassmix.cfg")', maps)
        self.assertIn('static void PWCollectMixedGrassStyleDependencies()', maps)
        self.assertGreaterEqual(maps.count('PWCollectMixedGrassStyleDependencies();'), 4)
        self.assertIn('PWTextureStyle::CreateFromTexture(', env)
        self.assertIn('Choose Sky Texture', env)
        self.assertIn('Choose Grass Texture', env)
        self.assertNotIn('importer_loadmodel();', env)
        self.assertNotIn('Import Sky Mesh', env)
        self.assertNotIn('Import Grass Mesh', env)
        self.assertIn('const std::string meshLeaf = sky ? name + Extension(mesh)', clone)
        self.assertIn('pw_sky_texture', clone)
        self.assertIn('SkyTexturePath(', sky)
        self.assertIn('grassFile = grassBase + ".jpg"', veg)
        self.assertIn('GLB importing is temporarily disabled', IMPORTER)
        self.assertNotIn('GLB\\0*.glb', GRID)
        self.assertNotIn('bPermittedFormat = true;\n                            if (stricmp(pExtension, ".glb")', GRID)

    def test_mixed_grass_does_not_overlap_classic_or_flicker_during_shadow_pass(self):
        terrain = (ROOT / "GameGuru Core/GameGuru/Source/M-Terrain.cpp").read_text(encoding="utf-8")
        grass = (ROOT / "GameGuru Core/GameGuru/Source/M-Grass.cpp").read_text(encoding="utf-8")
        mix = (ROOT / "GameGuru Core/GameGuru/Include/PW-GrassMix.h").read_text(encoding="utf-8")

        # Three independent Classic restoration sites exist: editor rebuild,
        # painted vegetation refresh, and the post-shadow render pass.
        # Never hide Classic grass just because styles loaded successfully.
        # A mixed renderer owns the region only AFTER some real blades exist.
        self.assertGreaterEqual(terrain.count("PWGrassMix::OwnsGrid()"), 3)
        self.assertIn("SetVegetationGridVisible(true);", grass)
        self.assertIn("if (PWGrassMix::OwnsGrid())", GRID)
        self.assertIn("Mix has no populated objects; Classic grass remains visible.", terrain)
        self.assertIn("Mix assets unavailable or not prepared.", terrain)
        self.assertIn("bool OwnsGrid();", mix)
        self.assertIn("bool PreviewEnabled();", mix)
        self.assertIn("bool RenderEnabled();", mix)
        self.assertIn("void SetPreviewEnabled(bool enabled);", mix)
        self.assertIn("static bool previewEnabled = false;", grass)
        self.assertIn("previewEnabled && Active()", grass)
        self.assertIn("PWGrassMix::RenderEnabled() && t.visuals.vegetationmode", grass)
        self.assertIn('Preview experimental multi-grass##PWSafeGrassPreview', terrain)
        self.assertIn("Safe mode: Classic grass remains visible; selected mix is saved.", terrain)
        self.assertIn("PWGrassMix::SetPreviewEnabled(pwPreview);", terrain)
        self.assertIn("PWGrassMix::Diagnostics();", terrain)
        self.assertIn("previewEnabled = false;\n    ResetInstances();", grass)
        self.assertIn("if (!RenderEnabled()) return;", grass)
        self.assertIn('lastIssue = "painted grass mask is not loaded";', grass)
        self.assertIn('lastIssue = "missing veg.DBO/veg.X for " + slots[i].style;', grass)
        self.assertIn('lastIssue = "could not load grass atlas for " + slots[i].style;', grass)
        self.assertIn('out << " | issue: " << lastIssue;', grass)
        self.assertIn("bool OwnsGrid() {", grass)
        self.assertIn("if (render[i].used > 0) return true;", grass)
        self.assertIn("if (PWGrassMix::OwnsGrid())", grass)
        self.assertNotIn("PWGrassMix::Active() && PWGrassMix::Ready()", terrain)
        self.assertNotIn("SetVegetationGridVisible(!PWGrassMix::Active())", grass)

        # Camera cropping previously sampled only 500 units around the
        # editor camera and a single mask pixel every two painted pixels.
        self.assertIn("for (int ring = 0; ring <= 48", grass)
        self.assertIn("const int nextX = offset + 4;", grass)
        self.assertIn("const int nextZ = offset + MAXTEXTURESIZE * 4;", grass)
        self.assertIn("ReadMemblockByte(memblockId, nextZ + 4)", grass)

        # Classic hides its vegetation during the shadow pass. Mixed objects
        # must be hidden too, then restored only while the mixed renderer owns
        # the painted grass region.
        self.assertIn("PWGrassMix::SetInstancesVisible(false);", terrain)
        self.assertIn("PWGrassMix::SetInstancesVisible(true);", terrain)
        self.assertIn("void SetInstancesVisible(bool visible);", mix)
        self.assertIn("if (visible && j < rs.used)", grass)

        # Each grass choice loads its own actual mesh/texture, not the current
        # one-style atlas from the native vegetation renderer.
        self.assertIn('const std::string dir = std::string(', grass)
        self.assertIn("LoadObject((LPSTR)model.c_str(), rs.source);", grass)
        self.assertIn("LoadImage((LPSTR)atlas.c_str(), rs.image);", grass)
        self.assertIn("TextureObject(rs.source, rs.image);", grass)
        self.assertIn("InstanceObject(id, rs.source);", grass)
        # Classic grass templates are 200% scaled. InstanceObject creates
        # independent transforms; the scale/yaw must reach each visible instance.
        self.assertIn("rs.scale = 200.f * opt.percent / 100.f;", grass)
        self.assertIn("rs.yaw = opt.yaw;", grass)
        self.assertIn("ScaleObject(id, rs.scale, rs.scale, rs.scale);", grass)
        self.assertIn("YRotateObject(inst, rs.yaw +", grass)

        # Hiding vegetation in the editor also releases PW-created instances.
        self.assertIn("PWGrassMix::ResetInstances();", terrain)

    def test_asset_inspector_keeps_docked_browser_and_uses_real_previews(self):
        inspector = (ROOT / "GameGuru Core/GameGuru/Include/PW-AssetInspector.h").read_text(encoding="utf-8")
        self.assertIn('Asset Inspector...##PWOpenAssetInspector', GRID)
        self.assertIn('PWAssetInspector::Draw();', GRID)
        self.assertIn('PWAssetInspector::RenderPending();', GRID)
        self.assertIn('ImGuiWindowFlags_NoDocking', inspector)
        self.assertIn('DrawTree(s.root,"Files",0);', inspector)
        self.assertIn('ImGui::Image((ImTextureID)view', inspector)
        self.assertIn('ImGui::IsMouseDragging(0)', inspector)
        self.assertIn('io.MouseWheel', inspector)
        self.assertIn('LoadFBX(', inspector)
        self.assertIn('LoadObject(', inspector)
        self.assertIn('editor_constructionselection();', inspector)
        self.assertIn('gridedit_addentitytomap();', inspector)
        self.assertIn('NewXValue(CameraPositionX(),CameraAngleY(),distance)', inspector)
        self.assertIn('NewZValue(CameraPositionZ(),CameraAngleY(),distance)', inspector)
        self.assertIn('entity_adduniqueentity(false);', inspector)
        self.assertNotIn('DockBuilderDockWindow("Asset Inspector', GRID)

    def test_shutdown_trace_records_cleanup_milestones(self):
        self.assertIn('PWShutdownTrace("file-exit accepted")', GRID)
        self.assertIn('PWShutdownTrace("finish entered; before common_justbeforeend")', GRID)
        self.assertIn('PWShutdownTrace("common_justbeforeend returned; before prefs")', GRID)

    def test_no_fake_toolbar_generation(self):
        self.assertNotIn('DrawPWIcon(', UPDATER)
        self.assertIn('ReportUnrecoveredClassicArtwork(built, steam);', UPDATER)
        self.assertIn('RestoreNativeClassicToolbar(built, steam, installFiles);', UPDATER)


if __name__ == "__main__":
    unittest.main()
