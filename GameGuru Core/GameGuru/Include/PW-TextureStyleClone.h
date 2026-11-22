#pragma once
// Pixelwolf Edition: create a Classic sky/grass style from ONE user-selected
// image. Reuse the currently selected native style's geometry/configuration.
// No FBX/DBO picker, mesh editor, or user-facing geometry adjustments.
#include "PW-StyleImport.h"
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace PWTextureStyle {
inline bool TextureExtension(const std::string& p) {
    const char* extensions[] = { ".dds", ".png", ".jpg", ".jpeg", ".bmp", ".tga" };
    for (const char* ext : extensions)
        if (PWImport::EndsWithI(p.c_str(), ext)) return true;
    return false;
}
inline std::string Extension(const std::string& p) {
    const size_t pos = p.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = p.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}
inline std::string StyleNameFromImage(const std::string& path) {
    std::string leaf = PWImport::BaseName(path);
    const size_t dot = leaf.find_last_of('.');
    if (dot != std::string::npos) leaf.resize(dot);
    std::string name = "PW_";
    for (char c : leaf) {
        if (name.size() >= 64) break;
        const unsigned char ch = static_cast<unsigned char>(c);
        name.push_back((std::isalnum(ch) || c == ' ' || c == '_' || c == '-') ? c : '_');
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '_')) name.pop_back();
    if (name == "PW" || name == "PW_") name = "PW_Custom";
    return name;
}
inline bool SafeStyleReference(const std::string& relative) {
    if (relative.empty() || relative.size() > 160 || relative[0] == '\\' ||
        relative[0] == '/' || relative.find(':') != std::string::npos) return false;
    size_t start = 0;
    while (start < relative.size()) {
        const size_t end = relative.find_first_of("\\/", start);
        const std::string piece = relative.substr(start, end == std::string::npos ? end : end - start);
        if (!PWStyleImport::SafeName(piece)) return false;
        if (end == std::string::npos) return true;
        start = end + 1;
    }
    return false;
}
inline std::string LastSegment(const std::string& relative) {
    const size_t p = relative.find_last_of("\\/");
    return relative.substr(p == std::string::npos ? 0 : p + 1);
}
inline bool IsModel(const std::string& f, const std::string& stem) {
    return PWStyleImport::Same(f, stem + ".dbo") || PWStyleImport::Same(f, stem + ".x");
}
inline bool IsGrassAtlas(const std::string& f) {
    const std::string ext = Extension(f);
    return PWStyleImport::Same(f.substr(0, f.size() - ext.size()), "grass") &&
           TextureExtension(f);
}
inline bool IsSkyOverride(const std::string& f) {
    const std::string ext = Extension(f);
    return PWStyleImport::Same(f.substr(0, f.size() - ext.size()), "pw_sky_texture") &&
           TextureExtension(f);
}
#ifdef _WIN32
inline bool CreateFromTexture(bool sky, const std::string& texture,
                              const std::string& filesRoot,
                              const std::string& selectedStyle,
                              std::string& createdName, std::string& error) {
    createdName.clear();
    error.clear();
    if (!TextureExtension(texture) || !PWStyleImport::RegularFile(texture)) {
        error = "Choose an existing PNG, DDS, JPG, JPEG, BMP or TGA image.";
        return false;
    }
    if (!SafeStyleReference(selectedStyle)) {
        error = "Select an existing sky or grass style in Terrain Tools first.";
        return false;
    }
    const std::string bank = filesRoot + (sky ? "\\skybank" : "\\vegbank");
    const std::string source = bank + "\\" + selectedStyle;
    if (!PWStyleImport::Directory(bank) || !PWStyleImport::Directory(source)) {
        error = "Selected built-in style folder is unavailable.";
        return false;
    }
    const std::string stem = LastSegment(selectedStyle);
    const std::string existingDbo = source + "\\" + (sky ? stem + ".dbo" : "veg.dbo");
    const std::string existingX = source + "\\" + (sky ? stem + ".x" : "veg.x");
    const std::string mesh = PWStyleImport::RegularFile(existingDbo) ? existingDbo : existingX;
    if (!PWStyleImport::RegularFile(mesh)) {
        error = "The selected style has no native DBO/X mesh to clone.";
        return false;
    }
    if (sky && !PWStyleImport::RegularFile(source + "\\skyspec.txt")) {
        error = "Sky template is missing skyspec.txt.";
        return false;
    }
    const std::string base = StyleNameFromImage(texture);
    std::string name;
    for (int i = 1; i <= 999; ++i) {
        const std::string candidate = i == 1 ? base : (base + "_" + std::to_string(i));
        if (PWStyleImport::SafeName(candidate) &&
            GetFileAttributesA((bank + "\\" + candidate).c_str()) == INVALID_FILE_ATTRIBUTES) {
            name = candidate; break;
        }
    }
    if (name.empty()) { error = "No unused custom style name is available."; return false; }
    const std::string destination = bank + "\\" + name;
    const std::string staging = bank + "\\pw_import_" + std::to_string(GetCurrentProcessId()) +
                                "_" + std::to_string(GetTickCount64());
    if (staging.size() > MAX_PATH - 88 || destination.size() > MAX_PATH - 88) {
        error = "Custom style folder path is too long."; return false;
    }
    if (!CreateDirectoryA(staging.c_str(), nullptr)) {
        error = "Could not create a temporary folder in the GameGuru style bank.";
        return false;
    }
    bool ok = true;
    std::vector<std::string> copied;
    std::uint64_t totalBytes = 0;
    WIN32_FIND_DATAA data = {};
    HANDLE find = FindFirstFileA((source + "\\*").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) { error = "Cannot read selected style."; ok = false; }
    if (find != INVALID_HANDLE_VALUE) {
        do {
            const std::string leaf = data.cFileName;
            if (leaf == "." || leaf == "..") continue;
            if (data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) {
                error = "Selected template contains a nested folder/link not supported by safe cloning.";
                ok = false; break;
            }
            if (!PWStyleImport::AllowedAsset(leaf)) continue; // Ignore non-runtime source notes.
            if (IsModel(leaf, sky ? stem : "veg") ||
                (sky ? IsSkyOverride(leaf) : IsGrassAtlas(leaf))) continue;
            totalBytes += (std::uint64_t(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
            if (copied.size() >= 256 || totalBytes > 512ULL * 1024 * 1024) {
                error = "Selected template is too large to clone safely."; ok = false; break;
            }
            if (!CopyFileA((source + "\\" + leaf).c_str(), (staging + "\\" + leaf).c_str(), TRUE)) {
                error = "Cannot copy template file: " + leaf; ok = false; break;
            }
            copied.push_back(leaf);
        } while (FindNextFileA(find, &data));
        FindClose(find);
    }
    const std::string meshLeaf = sky ? name + Extension(mesh) : "veg" + Extension(mesh);
    if (ok && !CopyFileA(mesh.c_str(), (staging + "\\" + meshLeaf).c_str(), TRUE)) {
        error = "Could not clone native style geometry."; ok = false;
    } else if (ok) copied.push_back(meshLeaf);
    const std::string imageLeaf = (sky ? "pw_sky_texture" : "grass") + Extension(texture);
    if (ok && !CopyFileA(texture.c_str(), (staging + "\\" + imageLeaf).c_str(), TRUE)) {
        error = "Could not copy selected image."; ok = false;
    } else if (ok) copied.push_back(imageLeaf);
    if (ok && !MoveFileA(staging.c_str(), destination.c_str())) {
        error = "Could not publish custom style; source styles were not changed.";
        ok = false;
    }
    if (!ok) { PWStyleImport::RemoveStaging(staging, copied); return false; }
    createdName = name;
    return true;
}
#endif
inline std::string SkyTexturePath(const std::string& styleFolder) {
    const char* extensions[] = { ".dds", ".png", ".jpg", ".jpeg", ".bmp", ".tga" };
    for (const char* ext : extensions) {
        const std::string name = styleFolder + "\\pw_sky_texture" + ext;
        if (PWStyleImport::RegularFile(name)) return name;
    }
    return "";
}
} // namespace PWTextureStyle
