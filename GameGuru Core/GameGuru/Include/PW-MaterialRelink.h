#pragma once
// Pixelwolf Model Importer 2 (M2): safe, non-destructive material slot helpers.
// The UI calls Classic's existing material application only after a replacement
// texture has loaded into a fresh image ID. This helper never mutates a model.
#include "PW-ImportPaths.h"
#include <string>

namespace PWMaterialRelink {

inline bool ValidSlot(int slot, int usedSlots, int maxSlots) {
    return slot > 0 && usedSlots > 0 && maxSlots > 1
        && slot <= usedSlots && slot < maxSlots;
}

inline bool SupportedTexture(const std::string& name) {
    return PWImport::EndsWithI(name.c_str(), ".dds")
        || PWImport::EndsWithI(name.c_str(), ".png")
        || PWImport::EndsWithI(name.c_str(), ".jpg")
        || PWImport::EndsWithI(name.c_str(), ".jpeg")
        || PWImport::EndsWithI(name.c_str(), ".tga")
        || PWImport::EndsWithI(name.c_str(), ".bmp");
}

inline std::string SourceFolder(const std::string& modelPath) {
    const std::string::size_type pos = modelPath.find_last_of("/\\");
    return pos == std::string::npos ? std::string() : modelPath.substr(0, pos + 1);
}

template <class Exists>
inline std::string Suggest(const std::string& reference,
                           const std::string& modelPath,
                           const std::string& converterFolder,
                           Exists exists) {
    if (reference.empty()) return std::string();
    // Prefer an existing source file or same-folder DDS-first companion;
    // the original importer's converter folder is only a fallback.
    const std::string source = PWImport::FindTexture(SourceFolder(modelPath), reference, exists);
    if (!source.empty()) return source;
    return PWImport::FindTexture(converterFolder, reference, exists);
}

} // namespace PWMaterialRelink
