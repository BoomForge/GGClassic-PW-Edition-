#pragma once
// Pixelwolf Model Importer 2 / Character Creator 2 shared PRE-FLIGHT contract.
// Read-only inspection of an already-loaded native GameGuru object. This does
// not convert bones, replace material stages, save meshes, or retarget clips.
// The schema is intentionally versioned so future tools may consume the report.
#include "PW-ImportPaths.h"
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace PWAssetManifest {

static const int kSchemaVersion = 1;
static const int kMaxMeshRecords = 10000;
static const int kMaxFrameRecords = 20000;
static const int kMaxTextureRecords = 50000;
static const int kMaxClipRecords = 4096;
static const int kMaxWarnings = 128;

struct MeshRecord {
    int index = 0;
    unsigned int vertices = 0;
    unsigned int bones = 0;
    unsigned int materialStages = 0;
};
struct FrameRecord {
    int index = 0;
    std::string name;
    std::string parent;
};
struct TextureRecord {
    int mesh = 0;
    int stage = 0;
    std::string reference;
    std::string resolved;
    bool found = false;
};
struct ClipRecord {
    std::string name;
    unsigned long long durationTicks = 0;
    unsigned int tracks = 0;
};
struct Manifest {
    int schema = kSchemaVersion;
    std::string source;
    std::vector<MeshRecord> meshes;
    std::vector<FrameRecord> frames;
    std::vector<TextureRecord> textures;
    std::vector<ClipRecord> clips;
    std::vector<std::string> warnings;
    unsigned int unresolvedTextures = 0;
    unsigned int skinBoneReferences = 0;
    bool truncated = false;
};

inline void Warn(Manifest& m, const std::string& message) {
    if (m.warnings.size() < kMaxWarnings) m.warnings.push_back(message);
}
inline std::string Directory(const std::string& path) {
    const std::size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? std::string() : path.substr(0, pos + 1);
}
inline std::string SafeName(const char* name) {
    return name ? std::string(name) : std::string();
}
inline std::string Esc(const std::string& s) {
    std::string output;
    output.reserve(s.size() + 8);
    const char* hex = "0123456789abcdef";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') {
            output += '\\';
            output += (char)c;
        } else if (c == '\n') output += "\\n";
        else if (c == '\r') output += "\\r";
        else if (c == '\t') output += "\\t";
        else if (c < 32) {
            output += "\\u00";
            output += hex[c >> 4];
            output += hex[c & 15];
        } else output += (char)c;
    }
    return output;
}

// A caller-provided filesystem predicate makes this unit-testable without
// Steam assets, GPU contexts or modifying GameGuru's active texture registry.
// The secondary directory is the existing FBX converter's temp folder.
template <class TObject, class Exists>
inline Manifest Inspect(const TObject* object, const std::string& sourcePath,
                        const std::string& converterFolder, Exists exists) {
    Manifest m;
    m.source = sourcePath;
    if (!object) {
        Warn(m, "No loaded model to inspect.");
        return m;
    }
    const int meshCount = object->iMeshCount;
    const int frameCount = object->iFrameCount;
    if (meshCount < 0 || meshCount > kMaxMeshRecords ||
        frameCount < 0 || frameCount > kMaxFrameRecords) {
        m.truncated = true;
        Warn(m, "Model mesh/frame count exceeds preflight safety limit.");
    }
    const int safeMeshCount = (std::max)(0, (std::min)(meshCount, kMaxMeshRecords));
    const int safeFrameCount = (std::max)(0, (std::min)(frameCount, kMaxFrameRecords));
    const std::string sourceFolder = Directory(sourcePath);
    for (int i = 0; i < safeMeshCount; ++i) {
        const auto* mesh = object->ppMeshList ? object->ppMeshList[i] : nullptr;
        if (!mesh) {
            Warn(m, std::string("Null mesh #") + std::to_string(i));
            continue;
        }
        MeshRecord record;
        record.index = i;
        record.vertices = mesh->dwVertexCount;
        record.bones = mesh->dwBoneCount;
        record.materialStages = mesh->dwTextureCount;
        m.meshes.push_back(record);
        m.skinBoneReferences += record.bones;
        if (record.bones && !mesh->pBones)
            Warn(m, std::string("Missing bone buffer in mesh #") + std::to_string(i));
        if (record.materialStages && !mesh->pTextures)
            Warn(m, std::string("Missing material table in mesh #") + std::to_string(i));
        if (record.materialStages > kMaxTextureRecords)
            m.truncated = true;
        if (!mesh->pTextures) continue;
        for (unsigned int stage = 0; stage < record.materialStages; ++stage) {
            if ((int)m.textures.size() >= kMaxTextureRecords) {
                m.truncated = true;
                break;
            }
            TextureRecord t;
            t.mesh = i;
            t.stage = (int)stage;
            t.reference = SafeName(mesh->pTextures[stage].pName);
            if (!t.reference.empty()) {
                t.resolved = PWImport::FindTexture(sourceFolder, t.reference, exists);
                if (t.resolved.empty())
                    t.resolved = PWImport::FindTexture(converterFolder, t.reference, exists);
                t.found = !t.resolved.empty();
                if (!t.found) {
                    ++m.unresolvedTextures;
                    if (m.unresolvedTextures <= 12)
                        Warn(m, std::string("Unresolved texture: ") + t.reference);
                }
            }
            m.textures.push_back(t);
        }
    }
    for (int i = 0; i < safeFrameCount; ++i) {
        const auto* frame = object->ppFrameList ? object->ppFrameList[i] : nullptr;
        if (!frame) {
            Warn(m, std::string("Null frame #") + std::to_string(i));
            continue;
        }
        FrameRecord f;
        f.index = i;
        f.name = SafeName(frame->szName);
        f.parent = frame->pParent ? SafeName(frame->pParent->szName) : "";
        m.frames.push_back(f);
    }
    // Protect against excessive/cyclic corrupted native animation lists.
    int clipCount = 0;
    for (const auto* clip = object->pAnimationSet;
         clip && clipCount < kMaxClipRecords; clip = clip->pNext, ++clipCount) {
        ClipRecord c;
        c.name = SafeName(clip->szName);
        c.durationTicks = clip->ulLength;
        int trackCount = 0;
        for (const auto* track = clip->pAnimation;
             track && trackCount < kMaxFrameRecords;
             track = track->pNext, ++trackCount) {}
        c.tracks = (unsigned int)trackCount;
        if (trackCount == kMaxFrameRecords) {
            m.truncated = true;
            Warn(m, "Animation track scan exceeded safety limit.");
        }
        m.clips.push_back(c);
    }
    if (clipCount == kMaxClipRecords) {
        m.truncated = true;
        Warn(m, "Animation clip scan exceeded safety limit.");
    }
    if (m.truncated) Warn(m, "Preflight truncated by safety limits; export not validated.");
    return m;
}

// Portable copyable manifest for future shared importer/character workflows.
// This reports inspected data; it is NOT a native DBO conversion recipe.
inline std::string Json(const Manifest& m) {
    std::ostringstream o;
    o << "{\n  \"schema\": \"pw.asset-preflight\", \"version\": " << m.schema
      << ",\n  \"source\": \"" << Esc(m.source) << "\""
      << ",\n  \"truncated\": " << (m.truncated ? "true" : "false")
      << ",\n  \"unresolvedTextures\": " << m.unresolvedTextures
      << ",\n  \"skinBoneReferences\": " << m.skinBoneReferences
      << ",\n  \"meshes\": [";
    for (std::size_t i = 0; i < m.meshes.size(); ++i) {
        const auto& x = m.meshes[i];
        if (i) o << ',';
        o << "{\"index\":" << x.index << ",\"vertices\":" << x.vertices
          << ",\"bones\":" << x.bones << ",\"materialStages\":" << x.materialStages << "}";
    }
    o << "],\n  \"frames\": [";
    for (std::size_t i = 0; i < m.frames.size(); ++i) {
        const auto& x = m.frames[i];
        if (i) o << ',';
        o << "{\"index\":" << x.index << ",\"name\":\"" << Esc(x.name)
          << "\",\"parent\":\"" << Esc(x.parent) << "\"}";
    }
    o << "],\n  \"textures\": [";
    for (std::size_t i = 0; i < m.textures.size(); ++i) {
        const auto& x = m.textures[i];
        if (i) o << ',';
        o << "{\"mesh\":" << x.mesh << ",\"stage\":" << x.stage
          << ",\"reference\":\"" << Esc(x.reference)
          << "\",\"resolved\":\"" << Esc(x.resolved)
          << "\",\"found\":" << (x.found ? "true" : "false") << "}";
    }
    o << "],\n  \"clips\": [";
    for (std::size_t i = 0; i < m.clips.size(); ++i) {
        const auto& x = m.clips[i];
        if (i) o << ',';
        o << "{\"name\":\"" << Esc(x.name) << "\",\"durationTicks\":"
          << x.durationTicks << ",\"tracks\":" << x.tracks << "}";
    }
    o << "],\n  \"warnings\": [";
    for (std::size_t i = 0; i < m.warnings.size(); ++i) {
        if (i) o << ',';
        o << "\"" << Esc(m.warnings[i]) << "\"";
    }
    o << "]\n}";
    return o.str();
}

} // namespace PWAssetManifest
