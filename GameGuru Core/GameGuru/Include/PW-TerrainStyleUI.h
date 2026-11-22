#pragma once
// PW Edition: environment assets use the SAME native 3D model importer as
// regular FBX/DBO entities. The importer previews textured geometry; its save
// operation routes to skybank/vegbank instead of emitting an ordinary FPE.
#include "PW-StyleImport.h"
#include "PW-ImportPaths.h"
#include <string>
extern int g_pwEnvironmentImportTarget; // 0 entity, 1 sky, 2 grass

static void PWActivateImportedEnvironmentStyle(int type, const char* importedName)
{
    const std::string name = importedName ? importedName : "";
    if (type == 1)
    {
        sky_init(); // Refresh native skybank.
        for (int i = 1; i <= g.skymax; ++i)
        {
            if (!PWStyleImport::Same(t.skybank_s[i].Get(), name)) continue;
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
    else if (type == 2)
    {
        grass_initstyles();
        for (int i = 1; i <= g.vegstylemax; ++i)
        {
            if (!PWStyleImport::Same(t.vegstylebank_s[i].Get(), name)) continue;
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
    popup_text("Native style saved, but GameGuru did not list the new style.");
}

void PWActivateImportedEnvironmentStyleFromImporter(int type, const char* name)
{
    PWActivateImportedEnvironmentStyle(type, name);
}

static void PWImportTerrainStyle(bool skyStyle)
{
    if (t.importer.importerActive != 0)
    {
        popup_text("Close or save your current model import before opening another.");
        return;
    }
    cstr oldDir = GetDir();
#ifdef _WIN64
    cstr selected = openFileBox(
        "GameGuru 3D models\0*.fbx;*.dbo;*.x\0FBX model\0*.fbx\0DBO model\0*.dbo\0X model\0*.x\0",
        "", skyStyle ? "Import Sky Mesh" : "Import Grass Mesh",
        "fbx", IMPORTEROPENFILE);
#else
    cstr selected = openFileBox(
        "GameGuru 3D models|*.fbx;*.dbo;*.x|FBX model|*.fbx|DBO model|*.dbo|X model|*.x|",
        "", skyStyle ? "Import Sky Mesh" : "Import Grass Mesh",
        "fbx", IMPORTEROPENFILE);
#endif
    SetDir(oldDir.Get());
    if (selected == "Error" || selected.Len() == 0) return;
    if (!PWImport::EndsWithI(selected.Get(), ".fbx") &&
        !PWImport::EndsWithI(selected.Get(), ".dbo") &&
        !PWImport::EndsWithI(selected.Get(), ".x"))
    {
        popup_text("Select an FBX, DBO or X model. GLB is disabled for now.");
        return;
    }
    if (!PWStyleImport::RegularFile(selected.Get()))
    {
        popup_text("Selected model cannot be opened.");
        return;
    }
    g_pwEnvironmentImportTarget = skyStyle ? 1 : 2;
    t.timporterfile_s = selected;
    importer_loadmodel(); // The existing textured 3D model preview and controls.
    if (t.importer.importerActive != 1) g_pwEnvironmentImportTarget = 0;
}
