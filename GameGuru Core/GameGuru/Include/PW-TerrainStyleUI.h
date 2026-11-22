#pragma once
// Pixelwolf editor-only sky/grass style import command. Include after the
// terrain-editor globals; never call a Win32 picker from inside an ImGui widget.
#include "PW-StyleImport.h"
#include <string>

static void PWImportTerrainStyle(bool skyStyle)
{
    const cstr oldDir = GetDir();
    cstr selected = "";
#ifdef _WIN64
    if (skyStyle)
        selected = openFileBox("Classic sky specification\0skyspec.txt\0All Files\0*.*\0",
            "", "Select skyspec.txt in the sky style folder", "txt", IMPORTEROPENFILE);
    else
        selected = openFileBox("GameGuru vegetation model\0veg.dbo;veg.x\0All Files\0*.*\0",
            "", "Select veg.DBO or veg.X in the grass style folder", "dbo", IMPORTEROPENFILE);
#else
    if (skyStyle)
        selected = openFileBox("Classic sky specification|skyspec.txt|All Files|*.*|",
            "", "Select skyspec.txt in the sky style folder", "txt", IMPORTEROPENFILE);
    else
        selected = openFileBox("GameGuru vegetation model|veg.dbo;veg.x|All Files|*.*|",
            "", "Select veg.DBO or veg.X in the grass style folder", "dbo", IMPORTEROPENFILE);
#endif
    SetDir(oldDir.Get());
    if (selected == "Error" || selected.Len() == 0) return;
    std::string style, error;
    const std::string bank = std::string(g.fpscrootdir_s.Get()) + "\\Files\\" +
        (skyStyle ? "skybank" : "vegbank");
    if (!PWStyleImport::Install(selected.Get(), bank,
            skyStyle ? PWStyleImport::Kind::Sky : PWStyleImport::Kind::Grass,
            style, error))
    {
        popup_text((LPSTR)error.c_str());
        return;
    }

    if (skyStyle)
    {
        sky_init();
        for (int i = 1; i <= g.skymax; ++i)
        {
            if (!PWStyleImport::Same(t.skybank_s[i].Get(), style)) continue;
            g.skyindex = i;
            t.visuals.skyindex = i;
            t.gamevisuals.skyindex = i;
            t.visuals.sky_s = t.skybank_s[i];
            t.gamevisuals.sky_s = t.skybank_s[i];
            t.terrainskyspecinitmode = 0;
            sky_skyspec_init();
            t.sky.currenthour_f = 8.0f;
            t.sky.daynightprogress = 0;
            visuals_justshaderupdate();
            t.visuals.refreshskysettingsfromlua = true;
            cubemap_generateglobalenvmap();
            t.visuals.refreshskysettingsfromlua = false;
            g.projectmodified = 1;
            return;
        }
    }
    else
    {
        grass_initstyles();
        for (int i = 1; i <= g.vegstylemax; ++i)
        {
            if (!PWStyleImport::Same(t.vegstylebank_s[i].Get(), style)) continue;
            g.vegstyleindex = i;
            g.vegstyle_s = t.vegstylebank_s[i];
            t.visuals.vegetationindex = i;
            t.visuals.vegetation_s = t.vegstylebank_s[i];
            t.gamevisuals.vegetationindex = i;
            t.gamevisuals.vegetation_s = t.vegstylebank_s[i];
            grass_init();
            bUpdateVeg = true;
            iLastUpdateVeg = 0;
            g.projectmodified = 1;
            return;
        }
    }
    popup_text("Style copied, but the new style is not visible in GameGuru's bank list.");
}
