"""Static regression checks for PW Editor V6 (not a substitute for Windows UI testing)."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parent.parent
GRID = (ROOT / "GameGuru Core/GameGuru/Source/M-GridEdit.cpp").read_text(encoding="utf-8")
UPDATER = (ROOT / "tools/PWBootstrapper/Program.cs").read_text(encoding="utf-8")


class PWEditorUiRegressions(unittest.TestCase):
    def test_one_bottom_asset_dock(self):
        self.assertIn('PWEngineDockspaceV6', GRID)
        first = GRID.index("//#### Default dockspace setup")
        last = GRID.index('// Fully nested scene tree;', first)
        docking = GRID[first:last]
        self.assertEqual(
            docking.count('DockBuilderDockWindow("Asset Browser##PWAssetBrowser", dock_id_bottom);'), 1)
        self.assertNotIn('DockBuilderDockWindow(TABENTITYNAME', docking)
        self.assertNotIn('DockBuilderDockWindow("Entity Library##ExternalWindow"', docking)

    def test_no_legacy_current_level_entities_pane(self):
        self.assertIn("g_pwShowLegacyLevelEntityList = false;", GRID)
        self.assertIn("if (g_pwShowLegacyLevelEntityList && refresh_gui_docking > 0)", GRID)
        self.assertNotIn('ImGui::Begin("##Object Library ExternalWindow"', GRID)
        self.assertIn('ImGui::Begin("Asset Browser##PWAssetBrowser", NULL', GRID)
        self.assertIn('ImGui::SetNextWindowDockID(g_pwBottomAssetDockId, ImGuiCond_Always);', GRID)

    def test_classic_style_and_opaque_browser(self):
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
        self.assertNotIn("editor_filllibrary();", library)
        self.assertIn('textureId = 0; // No thumbnail:', library)

    def test_hierarchy_parent_and_lua_entrypoints_survive(self):
        hierarchy = (ROOT / "GameGuru Core/GameGuru/Source/PW-SceneHierarchy.h").read_text(encoding="utf-8")
        self.assertIn("PWSetSceneParent(int child, int parent)", hierarchy)
        self.assertIn("PWMoveEntityToFolder(int e, int folder)", hierarchy)
        self.assertIn("editor_rec_addchildrentorubberband(e);", hierarchy)
        self.assertIn("OpenEntityScript(GetActiveWindow()", hierarchy)
        self.assertIn('if (ImGui::Button("Edit Lua Script..."))', GRID)

    def test_no_fake_toolbar_generation(self):
        self.assertNotIn('DrawPWIcon(', UPDATER)
        self.assertIn('ReportUnrecoveredClassicArtwork(built, steam);', UPDATER)
        self.assertIn('RestoreNativeClassicToolbar(built, steam, installFiles);', UPDATER)


if __name__ == "__main__":
    unittest.main()
