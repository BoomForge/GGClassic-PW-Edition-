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
        last = GRID.index("// The hierarchy lists actual instances", first)
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

    def test_no_fake_toolbar_generation(self):
        self.assertNotIn('DrawPWIcon(', UPDATER)
        self.assertIn('ReportUnrecoveredClassicArtwork(built, steam);', UPDATER)
        self.assertIn('RestoreNativeClassicToolbar(built, steam, installFiles);', UPDATER)


if __name__ == "__main__":
    unittest.main()
