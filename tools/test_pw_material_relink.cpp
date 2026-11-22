#include "PW-MaterialRelink.h"
#include <cassert>
#include <iostream>
#include <set>
#include <string>

int main() {
    assert(PWMaterialRelink::ValidSlot(1, 3, 24));
    assert(PWMaterialRelink::ValidSlot(3, 3, 24));
    assert(!PWMaterialRelink::ValidSlot(0, 3, 24));
    assert(!PWMaterialRelink::ValidSlot(4, 3, 24));
    assert(!PWMaterialRelink::ValidSlot(24, 24, 24));
    assert(!PWMaterialRelink::ValidSlot(1, 0, 24));
    assert(PWMaterialRelink::SupportedTexture("MyDiffuse.DDS"));
    assert(PWMaterialRelink::SupportedTexture("a.PnG"));
    assert(PWMaterialRelink::SupportedTexture("a.jpg"));
    assert(PWMaterialRelink::SupportedTexture("a.JPEG"));
    assert(PWMaterialRelink::SupportedTexture("a.tga"));
    assert(PWMaterialRelink::SupportedTexture("a.bmp"));
    assert(!PWMaterialRelink::SupportedTexture("a.fbx"));
    assert(!PWMaterialRelink::SupportedTexture(""));

    const std::set<std::string> assets {
        "C:\\models\\body.dds",
        "C:\\models\\body.png",
        "C:\\models\\overlay.tga",
        "C:\\ggc\\importer\\temp\\eyes.png"
    };
    const auto exists = [&assets](const std::string& s) { return assets.count(s) != 0; };
    assert(PWMaterialRelink::Suggest(
        "Z:\\removed\\body.png", "C:\\models\\",
        "C:\\ggc\\importer\\temp\\", exists) == "C:\\models\\body.png");
    assert(PWMaterialRelink::Suggest(
        "Z:\\removed\\body.jpg", "C:\\models\\",
        "C:\\ggc\\importer\\temp\\", exists) == "C:\\models\\body.dds");
    assert(PWMaterialRelink::Suggest(
        "eyes.png", "C:\\models\\",
        "C:\\ggc\\importer\\temp\\", exists) == "C:\\ggc\\importer\\temp\\eyes.png");
    assert(PWMaterialRelink::Suggest(
        "missing.jpg", "C:\\models\\",
        "C:\\ggc\\importer\\temp\\", exists).empty());
    assert(PWMaterialRelink::Suggest(
        "", "C:\\models\\",
        "C:\\ggc\\importer\\temp\\", exists).empty());

    // The GameGuru source path is a directory, often already slash-terminated.
    assert(PWMaterialRelink::Suggest(
        "body.jpg", "C:\\models",
        "C:\\ggc\\importer\\temp\\", exists) == "C:\\models\\body.dds");
    std::cout << "PW Model Importer 2 M2 material relink preflight: PASS\n";
    return 0;
}
