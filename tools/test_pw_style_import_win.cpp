#include "PW-StyleImport.h"
#include "PW-TextureStyleClone.h"
#include <cassert>
#include <fstream>
#include <cstdio>
static void writeFile(const std::string& p) {
    std::ofstream out(p, std::ios::binary);
    assert(out.good()); out << "test asset"; assert(out.good());
}
int main() {
    assert(PWStyleImport::SafeName("Misty Valley"));
    assert(!PWStyleImport::SafeName("../escape"));
    assert(!PWStyleImport::SafeName("foo.bar"));
    assert(PWStyleImport::AllowedAsset("model.DBO"));
    assert(!PWStyleImport::AllowedAsset("install.exe"));
    std::string root = "pw-style-import-test-" + std::to_string(GetCurrentProcessId());
    assert(CreateDirectoryA(root.c_str(), nullptr));
    assert(CreateDirectoryA((root + "\\skybank").c_str(), nullptr));
    assert(CreateDirectoryA((root + "\\vegbank").c_str(), nullptr));
    std::string sky = root + "\\Misty Valley";
    std::string grass = root + "\\Summer Grass";
    assert(CreateDirectoryA(sky.c_str(), nullptr));
    assert(CreateDirectoryA(grass.c_str(), nullptr));
    writeFile(sky + "\\skyspec.txt");
    writeFile(sky + "\\Misty Valley.DBO");
    writeFile(grass + "\\veg.DBO");
    writeFile(grass + "\\grass.png");
    std::string name, error;
    assert(PWStyleImport::Install(sky + "\\skyspec.txt", root + "\\skybank",
                                  PWStyleImport::Kind::Sky, name, error));
    assert(name == "Misty Valley");
    assert(PWStyleImport::RegularFile(root + "\\skybank\\Misty Valley\\skyspec.txt"));
    assert(!PWStyleImport::Install(sky + "\\skyspec.txt", root + "\\skybank",
                                   PWStyleImport::Kind::Sky, name, error));
    assert(PWStyleImport::Install(grass + "\\veg.DBO", root + "\\vegbank",
                                  PWStyleImport::Kind::Grass, name, error));
    assert(PWStyleImport::RegularFile(root + "\\vegbank\\Summer Grass\\grass.png"));
    assert(PWTextureStyle::TemplateUsable(true, root, "Misty Valley"));
    assert(PWTextureStyle::TemplateUsable(false, root, "Summer Grass"));
    assert(!PWTextureStyle::TemplateUsable(true, root, "..\\\\escape"));
    assert(!PWTextureStyle::TemplateUsable(false, root, "Misty Valley"));
    // One image is all the user supplies: preserve the installed mesh and
    // sky/grass settings while creating a new style without changing sources.
    writeFile(root + "\\Cloud Image.PNG");
    writeFile(root + "\\Wildgrass.JPG");
    std::string made;
    assert(PWTextureStyle::CreateFromTexture(true, root + "\\Cloud Image.PNG", root,
                                               "Misty Valley", made, error));
    assert(made == "PW_Cloud Image");
    assert(PWStyleImport::RegularFile(root + "\\skybank\\" + made + "\\PW_Cloud Image.dbo"));
    assert(PWStyleImport::RegularFile(root + "\\skybank\\" + made + "\\skyspec.txt"));
    assert(PWTextureStyle::SkyTexturePath(root + "\\skybank\\" + made) ==
           root + "\\skybank\\PW_Cloud Image\\pw_sky_texture.png");
    assert(PWTextureStyle::CreateFromTexture(false, root + "\\Wildgrass.JPG", root,
                                               "Summer Grass", made, error));
    assert(made == "PW_Wildgrass");
    assert(PWStyleImport::RegularFile(root + "\\vegbank\\" + made + "\\veg.dbo"));
    assert(PWStyleImport::RegularFile(root + "\\vegbank\\" + made + "\\grass.jpg"));
    assert(PWStyleImport::RegularFile(root + "\\vegbank\\Summer Grass\\grass.png"));
    assert(PWTextureStyle::CreateFromTexture(false, root + "\\Wildgrass.JPG", root,
                                               "Summer Grass", made, error));
    assert(made == "PW_Wildgrass_2");
    assert(!PWTextureStyle::CreateFromTexture(true, root + "\\Cloud Image.PNG", root,
                                                "..\\escape", made, error));
    assert(!PWTextureStyle::CreateFromTexture(false, root + "\\Cloud Image.PNG.exe", root,
                                                "Summer Grass", made, error));
    for (const std::string& file : {
        root + "\\Cloud Image.PNG", root + "\\Wildgrass.JPG",
        root + "\\skybank\\PW_Cloud Image\\PW_Cloud Image.dbo",
        root + "\\skybank\\PW_Cloud Image\\skyspec.txt",
        root + "\\skybank\\PW_Cloud Image\\pw_sky_texture.png",
        root + "\\vegbank\\PW_Wildgrass\\veg.dbo",
        root + "\\vegbank\\PW_Wildgrass\\grass.jpg",
        root + "\\vegbank\\PW_Wildgrass_2\\veg.dbo",
        root + "\\vegbank\\PW_Wildgrass_2\\grass.jpg" })
        DeleteFileA(file.c_str());
    RemoveDirectoryA((root + "\\skybank\\PW_Cloud Image").c_str());
    RemoveDirectoryA((root + "\\vegbank\\PW_Wildgrass").c_str());
    RemoveDirectoryA((root + "\\vegbank\\PW_Wildgrass_2").c_str());
    for (const std::string& p : {
        sky + "\\skyspec.txt", sky + "\\Misty Valley.DBO",
        grass + "\\veg.DBO", grass + "\\grass.png",
        root + "\\skybank\\Misty Valley\\skyspec.txt",
        root + "\\skybank\\Misty Valley\\Misty Valley.DBO",
        root + "\\vegbank\\Summer Grass\\veg.DBO",
        root + "\\vegbank\\Summer Grass\\grass.png" }) DeleteFileA(p.c_str());
    RemoveDirectoryA(sky.c_str()); RemoveDirectoryA(grass.c_str());
    RemoveDirectoryA((root + "\\skybank\\Misty Valley").c_str());
    RemoveDirectoryA((root + "\\vegbank\\Summer Grass").c_str());
    RemoveDirectoryA((root + "\\skybank").c_str());
    RemoveDirectoryA((root + "\\vegbank").c_str());
    RemoveDirectoryA(root.c_str());
    puts("PW style import staging/collision/security checks passed");
    return 0;
}
