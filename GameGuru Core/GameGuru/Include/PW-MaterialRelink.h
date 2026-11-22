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

// GameGuru's importer stores objectFileOriginalPath as a DIRECTORY and keeps
// objectFilename separately. Do not strip the last path segment as if the
// directory were a model filename (that can search in its parent instead).
template <class Exists>
inline std::string Suggest(const std::string& reference,
                           const std::string& originalDirectory,
                           const std::string& converterDirectory,
                           Exists exists) {
    if (reference.empty()) return std::string();
    // Prefer an existing source file or same-folder DDS-first companion;
    // the original importer's converter folder is only a fallback.
    const std::string source = PWImport::FindTexture(originalDirectory, reference, exists);
    if (!source.empty()) return source;
    return PWImport::FindTexture(converterDirectory, reference, exists);
}

} // namespace PWMaterialRelink
