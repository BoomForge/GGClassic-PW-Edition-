#pragma once
// PW Edition: texture-only native sky and grass style creation.
#include "PW-StyleImport.h"
#include "PW-TextureStyleClone.h"
#include "PW-ImportPaths.h"
#include <string>
extern int g_pwEnvironmentImportTarget; // 0 entity, 1 sky, 2 grass
extern char g_pwEnvironmentName[128];
extern float g_pwEnvironmentScalePct;
extern float g_pwEnvironmentYawDeg;

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
    // The only external input is an image. Clone the CURRENTLY SELECTED native
    // sky/grass style (mesh, UVs, shader and configuration) behind the scenes.
    if (t.importer.importerActive != 0)
    {
        popup_text("Finish the current entity import first.");
        return;
    }
    // A current style is usually already active. If it is not a valid native
    // template (e.g. the special dynamic sky), choose the first usable one
    // silently: the user only supplies an image and never picks a mesh.
    const std::string filesRoot = std::string(g.fpscrootdir_s.Get()) + "\\Files";
    std::string templateStyle;
    if (skyStyle)
    {
        if (g.skyindex > 0 && g.skyindex <= g.skymax)
            templateStyle = t.skybank_s[g.skyindex].Get();
        if (!PWTextureStyle::TemplateUsable(true, filesRoot, templateStyle))
        {
            templateStyle.clear();
            for (int i = 1; i <= g.skymax; ++i)
            {
                const std::string candidate = t.skybank_s[i].Get();
                if (PWTextureStyle::TemplateUsable(true, filesRoot, candidate))
                {
                    templateStyle = candidate;
                    break;
                }
            }
        }
    }
    else
    {
        templateStyle = g.vegstyle_s.Get();
        if (!PWTextureStyle::TemplateUsable(false, filesRoot, templateStyle))
        {
            templateStyle.clear();
            for (int i = 1; i <= g.vegstylemax; ++i)
            {
                const std::string candidate = t.vegstylebank_s[i].Get();
                if (PWTextureStyle::TemplateUsable(false, filesRoot, candidate))
                {
                    templateStyle = candidate;
                    break;
                }
            }
        }
    }
    if (templateStyle.empty())
    {
        popup_text(skyStyle ? "No compatible native sky style was found."
                            : "No compatible native grass style was found.");
        return;
    }
    cstr oldDir = GetDir();
#ifdef _WIN64
    cstr selected = openFileBox(
        "Image textures\0*.png;*.dds;*.jpg;*.jpeg;*.bmp;*.tga\0PNG image\0*.png\0DDS image\0*.dds\0All files\0*.*\0",
        "", skyStyle ? "Choose Sky Texture" : "Choose Grass Texture",
        "png", IMPORTEROPENFILE);
#else
    cstr selected = openFileBox(
        "Image textures|*.png;*.dds;*.jpg;*.jpeg;*.bmp;*.tga|PNG image|*.png|DDS image|*.dds|All files|*.*|",
        "", skyStyle ? "Choose Sky Texture" : "Choose Grass Texture",
        "png", IMPORTEROPENFILE);
#endif
    SetDir(oldDir.Get());
    if (selected == "Error" || selected.Len() == 0) return;

    std::string created, error;
    if (!PWTextureStyle::CreateFromTexture(skyStyle, selected.Get(), filesRoot,
                                            templateStyle, created, error))
    {
        popup_text((LPSTR)error.c_str());
        return;
    }
    PWActivateImportedEnvironmentStyle(skyStyle ? 1 : 2, created.c_str());
    popup_text(skyStyle ? "Sky texture applied to a new native sky style."
                        : "Grass texture applied to a new native grass style.");
}
