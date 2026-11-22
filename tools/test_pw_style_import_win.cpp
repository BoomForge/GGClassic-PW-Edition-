#include "PW-StyleImport.h"
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
