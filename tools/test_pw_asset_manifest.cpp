#include "PW-AssetManifest.h"
#include <cassert>
#include <iostream>
#include <set>
#include <string>

struct Texture { const char* pName; };
struct Bone { const char* szName; };
struct Mesh {
    unsigned int dwVertexCount, dwBoneCount, dwTextureCount;
    Bone* pBones;
    Texture* pTextures;
};
struct Frame { const char* szName; Frame* pParent; };
struct Track { Track* pNext; };
struct Clip {
    const char* szName;
    unsigned int ulLength;
    Track* pAnimation;
    Clip* pNext;
};
struct Object {
    int iMeshCount, iFrameCount;
    Mesh** ppMeshList;
    Frame** ppFrameList;
    Clip* pAnimationSet;
};

int main() {
    const auto exists = [](const std::string& path) {
        static const std::set<std::string> files {
            "C:\\art\\diffuse.dds",
            "C:\\art\\skin.jpg",
            "C:\\game\\importer\\temp\\normal.png"
        };
        return files.count(path) != 0;
    };
    Texture textures[] = {
        { "Z:\\former-artist\\diffuse.png" },
        { "normal.png" },
        { "Z:\\former-artist\\skin.jpg" },
        { "missing.png" },
        { nullptr }
    };
    Bone bone{ "mixamorig:Hips" };
    Mesh mesh{ 4, 1, 5, &bone, textures };
    Mesh* meshes[]{ &mesh };
    Frame root{ "Root", nullptr };
    Frame head{ "Head\\\"Bone", &root };
    Frame* frames[]{ &root, &head };
    Track track{ nullptr };
    Clip clip{ "Idle", 30, &track, nullptr };
    Object object{ 1, 2, meshes, frames, &clip };

    const auto m = PWAssetManifest::Inspect(
        &object, "C:\\art\\hero.fbx", "C:\\game\\importer\\temp\\", exists);
    assert(m.schema == 1);
    assert(m.meshes.size() == 1 && m.meshes[0].vertices == 4);
    assert(m.frames.size() == 2 && m.frames[1].parent == "Root");
    assert(m.textures.size() == 5);
    assert(m.textures[0].found && m.textures[0].resolved == "C:\\art\\diffuse.dds");
    assert(m.textures[1].found && m.textures[1].resolved == "C:\\game\\importer\\temp\\normal.png");
    assert(m.textures[2].found && m.textures[2].resolved == "C:\\art\\skin.jpg");
    assert(!m.textures[3].found && m.unresolvedTextures == 1);
    assert(m.skinBoneReferences == 1 && m.clips.size() == 1);
    assert(m.clips[0].durationTicks == 30 && m.clips[0].tracks == 1);
    assert(!m.truncated);
    const auto json = PWAssetManifest::Json(m);
    assert(json.find("\\"version\\": 1") != std::string::npos);
    assert(json.find("Head\\\\\\\"Bone") != std::string::npos);
    assert(json.find("\\"found\\":false") != std::string::npos);
    assert(json.find("\\"unresolvedTextures\\": 1") != std::string::npos);

    const auto empty = PWAssetManifest::Inspect(
        (Object*)nullptr, "", "", exists);
    assert(!empty.warnings.empty() && empty.meshes.empty());
    Object corrupt{ 20000, 50000, nullptr, nullptr, nullptr };
    const auto bounded = PWAssetManifest::Inspect(&corrupt, "", "", exists);
    assert(bounded.truncated && bounded.meshes.empty() && bounded.frames.empty());
    assert(bounded.warnings.size() <= PWAssetManifest::kMaxWarnings);

    // Null tables in damaged assets must warn instead of dereferencing.
    Mesh broken{ 0, 3, 1, nullptr, nullptr };
    Mesh* brokenList[]{ &broken };
    Object bad{ 1, 0, brokenList, nullptr, nullptr };
    const auto report = PWAssetManifest::Inspect(&bad, "", "", exists);
    assert(report.warnings.size() >= 2 && report.skinBoneReferences == 3);

    std::cout << "PW Asset Manifest M1 preflight: PASS\\n";
}
