#pragma once
// GameGuru multi-material DBOs can embed _D.dds or *.dds references while the
// importer exports the actual image as PNG. Resolve only when DDS is missing.
// The renderer's native texture loader remains authoritative; no mesh mutation.
#include "PW-ImportPaths.h"
#include <string>
namespace PWMultiMaterialFallback {
template<class Exists>
inline std::string ResolveMissingDDS(const std::string& entityFolder,
                                      const std::string& embeddedReference,
                                      Exists exists) {
    if (!PWImport::EndsWithI(embeddedReference.c_str(), ".dds")) return "";
    const std::string resolved =
        PWImport::FindTexture(entityFolder, embeddedReference, exists);
    return PWImport::EndsWithI(resolved.c_str(), ".png") ? resolved : "";
}
}
