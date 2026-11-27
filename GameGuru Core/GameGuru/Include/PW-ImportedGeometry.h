#pragma once
// PW Edition: verify importer/asset entities have real renderable geometry.
// ObjectExist() alone is NOT proof that FBX/DBO loaded valid meshes.
// No changes to skeleton, textures, export file or engine object.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
namespace PWImportedGeometry {
struct Report {
    int meshes = 0;
    unsigned long long vertices = 0;
    int invalidMeshes = 0;
    bool usable = false;
    std::string error;
};
template<class Object>
inline Report Inspect(const Object* obj) {
    Report r;
    if (!obj) { r.error = "No model object was loaded."; return r; }
    if (obj->iMeshCount < 1 || obj->iMeshCount > 100000 || !obj->ppMeshList) {
        r.error = "Model has no valid mesh table."; return r;
    }
    r.meshes = obj->iMeshCount;
    for (int i = 0; i < obj->iMeshCount; ++i) {
        const auto* mesh = obj->ppMeshList[i];
        if (!mesh || mesh->dwVertexCount == 0 || mesh->dwVertexCount > 100000000) {
            ++r.invalidMeshes; continue;
        }
        r.vertices += mesh->dwVertexCount;
    }
    r.usable = r.vertices > 0;
    if (!r.usable) r.error = "Model contains no renderable vertices.";
    return r;
}
inline bool SensibleExtent(float x, float y, float z) {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z)
        && x >= 0 && y >= 0 && z >= 0
        && (std::max)({x, y, z}) > 0.00001f;
}
} // namespace PWImportedGeometry
