// PW Edition: source-only regression checks for Blender/Mixamo FBX texture paths.
#include "PW-ImportPaths.h"
#include <cassert>
#include <set>
#include <string>
#include <cstdio>

int main()
{
    const std::set<std::string> files = {
        "C:\\Models\\Hero\\body_color.png",
        "C:\\Models\\Hero\\hair_D.dds",
        "C:\\Models\\Hero\\Face_normal.tga",
        "C:\\Old\\old_texture.jpg"
    };
    auto exists = [&files](const std::string& path) { return files.find(path) != files.end(); };

    assert(PWImport::EndsWithI("avatar.FBX", ".fbx"));
    assert(!PWImport::EndsWithI("", ".fbx"));
    assert(!PWImport::EndsWithI("x", "_color"));
    assert(!PWImport::EndsWithI(nullptr, ".fbx"));
    assert(PWImport::BaseName("D:\\mixamo\\body_color.png") == "body_color.png");
    assert(PWImport::BaseName("body_color.png") == "body_color.png");
    assert(PWImport::ReplaceExtension("C:\\A\\foo.fbx", ".png") == "C:\\A\\foo.png");
    assert(PWImport::Folder("C:\\Models\\Hero") == "C:\\Models\\Hero\\");

    // Keep the exact original file, including a valid absolute reference.
    assert(PWImport::FindTexture("C:\\Models\\Hero", "C:\\Old\\old_texture.jpg", exists) == "C:\\Old\\old_texture.jpg");
    // Blender-generated absolute references may not exist on this user's PC:
    // retrieve the basename relative to the imported FBX folder instead.
    assert(PWImport::FindTexture("C:\\Models\\Hero", "D:\\Someone\\Exports\\body_color.png", exists) == "C:\\Models\\Hero\\body_color.png");
    // Missing PNG falls back to existing DDS (never removes DDS support).
    assert(PWImport::FindTexture("C:\\Models\\Hero", "Z:\\missing\\hair_D.png", exists) == "C:\\Models\\Hero\\hair_D.dds");
    assert(PWImport::FindTexture("C:\\Models\\Hero", "C:\\elsewhere\\Face_normal.jpg", exists) == "C:\\Models\\Hero\\Face_normal.tga");
    assert(PWImport::FindTexture("C:\\Models\\Hero", "no_match.png", exists).empty());
    assert(PWImport::FindTexture("C:\\Models\\Hero", "", exists).empty());

    puts("PW importer path contract checks passed.");
    return 0;
}
