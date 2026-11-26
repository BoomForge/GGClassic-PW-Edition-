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

    def test_explicit_player_setup_preserves_classic_map_contract(self):
        hierarchy = (ROOT / "GameGuru Core/GameGuru/Source/PW-SceneHierarchy.h").read_text(encoding="utf-8")
        maps = (ROOT / "GameGuru Core/GameGuru/Source/M-MapFile.cpp").read_text(encoding="utf-8")
        self.assertIn("PWRenderPlayerSetup();", hierarchy)
        self.assertIn("PWAssignScenePlayer(e, m)", hierarchy)
        self.assertIn("!PWPlayerSetupCandidate(character)", hierarchy)
        self.assertIn("!PWPlayerSetupEntity(marker, true)", hierarchy)
        self.assertIn("_stricmp(script, stock) == 0", hierarchy)
        self.assertIn("ent.staticflag != 0", hierarchy)
        self.assertIn("ent.iHasParentIndex != 0", hierarchy)
        self.assertIn("t.playercontrol.thirdperson.charactere = character;", hierarchy)
        self.assertIn("t.playercontrol.thirdperson.startmarkere = marker;", hierarchy)
        self.assertIn("t.playercontrol.thirdperson.enabled = 1;", hierarchy)
        self.assertIn("avatar.eleprof.aimain_s = t.entityprofile[avatar.bankindex].aimain_s;", hierarchy)
        self.assertIn("PWCanReturnToFirstPerson()", hierarchy)
        self.assertIn("t.playercontrol.thirdperson.enabled = 0;", hierarchy)
        self.assertIn("g.projectmodified = 1;", hierarchy)
        self.assertIn("WriteLong (  1,t.playercontrol.thirdperson.charactere );", maps)
        self.assertIn("WriteLong (  1,t.playercontrol.thirdperson.startmarkere );", maps)
        self.assertIn("WriteLong (  1,t.playercontrol.thirdperson.enabled );", maps)

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
        self.assertIn('PWGrassMix::PrepareNative();', veg)
        self.assertIn('SetPWVegetationMix(nativeCount,meshes,images,weights);', veg)
        self.assertNotIn('InstanceObject(id, rs.source);', veg)
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

    def test_mixed_grass_native_batches_share_grid_and_density(self):
        terrain = (ROOT / "GameGuru Core/GameGuru/Source/M-Terrain.cpp").read_text(encoding="utf-8")
        grass = (ROOT / "GameGuru Core/GameGuru/Source/M-Grass.cpp").read_text(encoding="utf-8")
        native = (ROOT / "GameGuru Core/Dark Basic Public Shared/Dark Basic Pro SDK/DarkSDKMore/SimonReloaded/SimonReloaded.cpp").read_text(encoding="utf-8")
        self.assertIn("PWGrassMix::PrepareNative();", grass)
        self.assertLess(grass.index("PWGrassMix::PrepareNative();"), grass.index("MakeVegetationGrid(4.0f"))
        self.assertIn("SetPWVegetationMix(nativeCount,meshes,images,weights);", grass)
        self.assertIn("MakeMeshFromObject(rs.mesh,rs.mesh);", grass)
        self.assertIn("const int total=iVegPerMesh+1;", native)
        self.assertIn("while(used<total)", native)
        self.assertIn("PWConfigureCounts();", native)
        self.assertIn("PWSpeciesScope scope(p,prototype); MakeVegPatchSingle", native)
        self.assertIn("UpdateVegPatchSingle(id,terrain,x,z,flat,height);", native)
        self.assertIn("TextureObject(id,0,pwSpeciesCount?pwSpecies[s].image:iGrassImg);", native)
        self.assertIn("PWShowPatch(iVegObj);", native)
        self.assertIn("PWHidePatch(iVegObj);", native)
        self.assertIn("PWDeleteExtraObjects();", native)
        self.assertIn("Show additional grass types##PWSafeGrassPreview", terrain)
        self.assertIn("previewEnabled = Active();", grass)
        self.assertNotIn("InstanceObject(id, rs.source);", grass)
        self.assertNotIn("count < 320", grass)

    def test_direct_projectile_damage_uses_swept_hit_and_bullet_semantics(self):
        weapon = (ROOT / "GameGuru Core/GameGuru/Source/M-Weapon.cpp").read_text(encoding="utf-8")
        self.assertIn("PWProjectile::FirstContact(", weapon)
        self.assertIn("if(t.ttdd_f>0.0001f && !pwDirect)", weapon)
        self.assertIn("if(!pwDirect)", weapon)
        self.assertIn("if(!ODERayTerrain(", weapon)
        direct = weapon.split("void weapon_projectileresult_make ( void )",1)[1].split("case WEAPON_PROJECTILERESULT_EXPLODE:",1)[0]
        self.assertIn("t.tdamagesource=1;", direct)
        self.assertNotIn("t.tdamagesource=2;", direct)
        self.assertIn("entity_applydamage ( );", direct)
        self.assertIn("t.te=oldEntity;", weapon)
        self.assertIn("physics_explodesphere ( );", weapon)

    def test_gameplay_controls_are_separate_from_raw_keymap(self):
        common = (ROOT / "GameGuru Core/GameGuru/Source/Common.cpp").read_text(encoding="utf-8")
        physics = (ROOT / "GameGuru Core/GameGuru/Source/M-Physics.cpp").read_text(encoding="utf-8")
        lua = (ROOT / "GameGuru Core/GameGuru/Source/M-LUA.cpp").read_text(encoding="utf-8")
        header = (ROOT / "GameGuru Core/GameGuru/Include/PW-GameplayControls.h").read_text(encoding="utf-8")
        self.assertIn("dialog.controls=PWOpenControls;", common)
        self.assertIn("pwControlsActive=PWControls::Read(root,pwControls);", common)
        self.assertIn("PWGameplayKey(17,g.keymap[t.plrkeyW])", physics)
        self.assertIn("PWGameplayKey(18,18)", lua)
        self.assertIn("!PWGameplayControlsActive() && ControlKey()", physics)
        self.assertIn("if(!Valid(next))", header)
        self.assertIn("PWProfile::Atomic", header)
        self.assertIn("Reset defaults", header)

    def test_standalone_display_options_preserve_legacy_startup(self):
        common = (ROOT / "GameGuru Core/GameGuru/Source/Common.cpp").read_text(encoding="utf-8")
        export = (ROOT / "GameGuru Core/GameGuru/Source/M-MapFile.cpp").read_text(encoding="utf-8")
        display = (ROOT / "GameGuru Core/GameGuru/Include/PW-DisplayOptions.h").read_text(encoding="utf-8")
        self.assertIn('GetPrivateProfileIntA("GAMERUN","pwdisplayoptions",0', common)
        self.assertIn("g.iStandaloneIsReloading==0&&!interrupted", common)
        self.assertIn("PWDisplay::MarkPending", common)
        self.assertIn("PWDisplay::Confirm", common)
        self.assertIn("if(!pwDisplayActive)", common)
        self.assertIn("SetDisplayModeMODBACKBUFFER(c.width,c.height", common)
        self.assertIn("SetWindowModeOn();", common)
        self.assertIn("EnumDisplaySettingsA", display)
        self.assertIn("d.seconds=15;", display)
        self.assertIn("PWProfile::Atomic", display)
        self.assertGreaterEqual(export.count('pwdisplayoptions=1'), 2)

    def test_custom_asset_removal_is_recoverable_and_scoped(self):
        self.assertIn('Delete imported asset...', GRID)
        self.assertIn('BeginPopupModal("Delete imported asset##PWRemoveConfirm"', GRID)
        self.assertIn('Files\\\\entitybank\\\\user', GRID)
        self.assertIn('FILE_ATTRIBUTE_REPARSE_POINT', GRID)
        self.assertIn('t.entityelement[e].bankindex==id', GRID)
        self.assertIn('id<=g.entidmaster', GRID)
        self.assertIn('return path+".pw-quarantine";', GRID)
        self.assertIn('if(!Validate(path,issue)) return false;', GRID)
        self.assertIn('MoveFileExA(path.c_str(),dest.c_str(),MOVEFILE_WRITE_THROUGH)', GRID)
        self.assertIn('RefreshEntityFolder(f->m_sFolderFullPath.Get(),f);', GRID)
        self.assertIn('Other saved maps have not been checked', GRID)
        removal = GRID.split('namespace PWAssetRemoval {',1)[1].split('void process_entity_library(void)',1)[0]
        self.assertNotIn('DeleteFile', removal)
        self.assertNotIn('MOVEFILE_REPLACE_EXISTING', removal)

    def test_asset_inspector_keeps_docked_browser_and_uses_real_previews(self):
        inspector = (ROOT / "GameGuru Core/GameGuru/Include/PW-AssetInspector.h").read_text(encoding="utf-8")
        self.assertIn('Asset Inspector...##PWOpenAssetInspector', GRID)
        self.assertIn('PWAssetInspector::Draw();', GRID)
        self.assertIn('PWAssetInspector::RenderPending();', GRID)
        self.assertIn('ImGuiWindowFlags_NoDocking', inspector)
        self.assertIn('DrawIndexedTree(s.root,"Files",Index().entries,0);', inspector)
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

    def test_asset_inspector_filters_setup_files_and_generated_thumbnails(self):
        inspector = (ROOT / "GameGuru Core/GameGuru/Include/PW-AssetInspector.h").read_text(encoding="utf-8")
        # Only native model formats appear in the new viewer. FBX and image
        # decoding may still be used internally or by other import workflows.
        self.assertIn('return Ends(p, ".dbo") || Ends(p, ".x");', inspector)
        self.assertIn("inline bool ViewableFile(const std::string& path)", inspector)
        self.assertIn("return ModelFile(path) && !ThumbnailName(path);", inspector)
        self.assertIn('n.find("thumbnail")', inspector)
        self.assertIn('n.find("_thumb")', inspector)
        self.assertIn('n!="thumbnails"', inspector)
        self.assertIn("} else if (ViewableFile(name)) {", inspector)
        self.assertIn('ImGui::Button("Refresh Models##PWAssetRefresh")', inspector)
        self.assertIn("InvalidateBrowser();", inspector)
        self.assertIn('DrawIndexedTree(s.root,"Files",Index().entries,0);', inspector)
        self.assertIn('TextureForModel(const std::string& model)', inspector)
        self.assertIn('FindFirstFileA((folder+"\\\\*.fpe").c_str(),&data)', inspector)
        self.assertIn('s.albedoSource=TextureForModel(file);', inspector)
        self.assertNotIn('return Ends(p, ".fpe") ||', inspector)

    def test_asset_inspector_scans_incrementally_and_searches_unique_models(self):
        inspector = (ROOT / "GameGuru Core/GameGuru/Include/PW-AssetInspector.h").read_text(encoding="utf-8")
        self.assertIn('#include <deque>', inspector)
        self.assertIn('inline void TickBrowserScan()', inspector)
        self.assertIn('processed<192 && GetTickCount()-frameStarted<6', inspector)
        self.assertIn('b.pending.push_back(std::move(child));', inspector)
        self.assertIn('FindNextFileA(b.find,&b.found)', inspector)
        self.assertIn('FinaliseBrowserScan(b);', inspector)
        self.assertIn('inline void SortPruneUnique(std::vector<Entry>& entries)', inspector)
        self.assertIn('Ends(a.name,".dbo")!=Ends(b.name,".dbo")', inspector)
        self.assertIn('entries.erase(std::unique(entries.begin(),entries.end()', inspector)
        self.assertIn('Lower(Stem(a.name))==Lower(Stem(b.name))', inspector)
        self.assertIn('ImGui::Selectable(Stem(e.name).c_str()', inspector)
        self.assertIn('ImGui::InputText("Search models##PWAssetSearch"', inspector)
        self.assertIn('DrawSearchResults(s.searchQuery);', inspector)
        self.assertIn('if (matches>200)', inspector)
        self.assertIn('Release(true); // Keep the compiled preview shader', inspector)
        self.assertIn('s.loadMilliseconds=GetTickCount()-loadStarted;', inspector)
        self.assertIn('if (s.root!=root) InvalidateBrowser();', inspector)
        self.assertIn('if (Index().scanning) InvalidateBrowser();', inspector)
        self.assertIn('if (!s.open) {', inspector)

    def test_inspector_frames_raw_3d_in_isolated_preview_with_shader(self):
        inspector = (ROOT / "GameGuru Core/GameGuru/Include/PW-AssetInspector.h").read_text(encoding="utf-8")
        # Preview temporary IDs must not collide with live GameGuru entity
        # or image allocations (old 26400 and 71750 were allocated banks).
        self.assertIn("const int base = 220000;", inspector)
        self.assertIn("const int base=120500;", inspector)
        self.assertIn("loadinternaleffectunique(basicShader,1)", inspector)
        self.assertIn("SetObjectEffect(s.modelId,s.modelEffectId)", inspector)
        self.assertIn("TextureObject(s.modelId,s.modelTextureId)", inspector)
        self.assertIn("deleteinternaleffect(s.modelEffectId)", inspector)
        self.assertIn("if (!ImageExist(s.previewId) || !GetImagePointerView(s.previewId))", inspector)
        # Old thumbnail renderer uses live terrain elevation, which can
        # occlude a raw model or leave the capture looking at empty land.
        self.assertIn("float terrain_height = inspectorRender ? 20000.0f", GRID)
        self.assertIn("if (inspectorRender) {", GRID)
        self.assertIn("const float radius=(std::max)(35.0f", GRID)
        self.assertIn("const float safeExtent=std::isfinite(extent)", GRID)
        self.assertIn("const float pitchedY = cy * std::cos(p)", GRID)
        self.assertIn("fOffsetX = cx * std::cos(y) + pitchedZ * std::sin(y);", GRID)
        self.assertIn("fOffsetZ = pitchedZ * std::cos(y) - cx * std::sin(y);", GRID)
        self.assertIn("const float extent=std::sqrt(fLargestX*fLargestX", GRID)
        self.assertIn('ImGui::InvisibleButton("##PWAssetModelOrbit"', inspector)
        self.assertIn("const bool previewActive=ImGui::IsItemActive();", inspector)
        self.assertIn("if (previewActive && ImGui::IsMouseDragging(0))", inspector)
        self.assertIn("ImGui::GetWindowDrawList()->AddImage((ImTextureID)view", inspector)
        self.assertIn("ImGui::SetCursorPosX(ImGui::GetCursorPosX()+leftPadding);", inspector)
        self.assertNotIn("if (ImGui::IsMouseDragging(0)) {", inspector)
        # Catch accidental extra parentheses in the raw-model camera radius:
        # the previous mistake passed textual guards but broke MSVC C2059.
        radius_start = GRID.index("const float radius=(std::max)")
        radius_end = GRID.index(";", radius_start)
        radius_expr = GRID[radius_start:radius_end]
        balance = 0
        for char in radius_expr:
            if char == "(":
                balance += 1
            elif char == ")":
                balance -= 1
                self.assertGreaterEqual(balance, 0, "Inspector camera radius closes too many parentheses")
        self.assertEqual(balance, 0, "Inspector camera radius has unclosed parentheses")
        self.assertIn("terrain_height + (inspectorRender ? fOffsetY : 0.0f)", GRID)
        self.assertIn("if (!inspectorRender && t.terrain.TerrainID > 0)", GRID)
        self.assertIn("if (!inspectorRender) {", GRID)
        self.assertIn("GrabImage((inspectorRender ? previewImageId", GRID)
        self.assertIn("PWAssetInspector::RenderPending();", GRID)
        self.assertIn('ImGui::Button("Refresh 3D Preview")', inspector)
        self.assertIn("s.renderRequested=true;", inspector)

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
