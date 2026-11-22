#pragma once
// PW Edition: save a model already previewed/edited by GameGuru's FBX/DBO
// importer as a native sky or vegetation style, not a placeable FPE entity.
// Included by M-Importer.cpp after gameguru.h / importer globals.
#include "PW-StyleImport.h"
#include "PW-ImportedGeometry.h"
#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace PWEnvironment {
enum Target { Entity = 0, Sky = 1, Grass = 2 };
inline bool RealFile(const std::string& p) { return PWStyleImport::RegularFile(p); }
inline bool CopyTexture(const std::string& source, const std::string& dest, std::string& error) {
    if (!RealFile(source)) { error = "Missing texture file: " + source; return false; }
    if (!CopyFileA(source.c_str(), dest.c_str(), TRUE)) {
        error = "Could not copy texture: " + PWImport::BaseName(source); return false;
    }
    return true;
}
inline std::string PickTexturePath(const std::string& reference, const std::string& sourceFolder) {
    const auto exists = [](const std::string& p) { return RealFile(p); };
    return PWImport::FindTexture(sourceFolder, reference, exists);
}
inline bool Save(int target, const char* requestedName, int modelObject,
                 const std::vector<std::string>& textureFiles,
                 const std::string& sourceFolder, const std::string& filesRoot,
                 float sizePercent, float yawDeg, std::string& error) {
    error.clear();
    if ((target != Sky && target != Grass) || !requestedName) {
        error = "Choose the sky or grass importer first."; return false;
    }
    const std::string name = requestedName;
    if (!PWStyleImport::SafeName(name)) {
        error = "Enter a style name with letters, numbers, spaces, hyphens or underscores.";
        return false;
    }
    const PWImportedGeometry::Report valid = PWImportedGeometry::Inspect(
        ObjectExist(modelObject) ? GetObjectData(modelObject) : nullptr);
    if (!valid.usable) { error = "Model has no usable mesh geometry."; return false; }
    if (sizePercent < 1.f || sizePercent > 400.f || yawDeg < -180.f || yawDeg > 180.f) {
        error = "Invalid size or yaw setting."; return false;
    }
    const std::string bank = filesRoot + (target == Sky ? "\\skybank" : "\\vegbank");
    if (!PWStyleImport::Directory(bank)) {
        error = "Native GameGuru skybank/vegbank directory is missing."; return false;
    }
    const std::string dest = bank + "\\" + name;
    if (GetFileAttributesA(dest.c_str()) != INVALID_FILE_ATTRIBUTES) {
        error = "A style with that name already exists. Nothing was overwritten."; return false;
    }
    const std::string stage = bank + "\\pw_import_" +
        std::to_string(GetCurrentProcessId()) + "_" + std::to_string(GetTickCount64());
    if (dest.size() > MAX_PATH - 88 || stage.size() > MAX_PATH - 88) {
        error = "Style path is too long."; return false;
    }

    // Validate texture sources before touching the destination bank.
    if (textureFiles.empty()) { error = "No material texture assigned. Choose an albedo texture in the importer."; return false; }
    std::vector<std::string> resolved;
    std::set<std::string> names;
    for (const std::string& filename : textureFiles) {
        if (filename.empty()) continue;
        const std::string found = PickTexturePath(filename, sourceFolder);
        if (found.empty()) { error = "Cannot resolve material file: " + filename; return false; }
        const std::string leaf = PWImport::BaseName(found);
        if (!PWStyleImport::AllowedAsset(leaf) || leaf == "." || leaf == "..") {
            error = "Unsafe texture file name: " + leaf; return false;
        }
        if (!names.insert(leaf).second) continue;
        resolved.push_back(found);
    }
    if (resolved.empty()) { error = "No readable material images were found."; return false; }
    std::string grassSource;
    if (target == Grass) {
        for (const std::string& f : resolved) {
            if (PWImport::EndsWithI(f.c_str(), ".png") ||
                PWImport::EndsWithI(f.c_str(), ".dds")) { grassSource = f; break; }
        }
        if (grassSource.empty()) {
            error = "Grass requires a PNG or DDS albedo atlas. Assign one in Materials & Relinking.";
            return false;
        }
    }
    if (!CreateDirectoryA(stage.c_str(), nullptr)) {
        error = "Cannot create a staging folder in the GameGuru style bank."; return false;
    }
    std::vector<std::string> stagedFiles;
    bool ok = true;
    for (const std::string& f : resolved) {
        const std::string leaf = PWImport::BaseName(f);
        if (!CopyTexture(f, stage + "\\" + leaf, error)) { ok = false; break; }
        stagedFiles.push_back(leaf);
    }
    if (ok && target == Grass) {
        const std::string grassAtlas = PWImport::EndsWithI(grassSource.c_str(), ".dds")
            ? "grass.dds" : "grass.png";
        if (!PWStyleImport::Same(PWImport::BaseName(grassSource), grassAtlas)) {
            if (!CopyTexture(grassSource, stage + "\\" + grassAtlas, error)) ok = false;
            else stagedFiles.push_back(grassAtlas);
        }
    }
    if (ok) {
        const std::string modelLeaf = target == Sky ? (name + ".dbo") : "veg.dbo";
        const std::string mesh = stage + "\\" + modelLeaf;
        std::vector<char> mutableMesh(mesh.begin(), mesh.end());
        mutableMesh.push_back(0);
        SaveObject(mutableMesh.data(), modelObject);
        if (!RealFile(mesh)) {
            error = "GameGuru failed to export the native model DBO.";
            ok = false;
        } else stagedFiles.push_back(modelLeaf);
    }
    if (ok && target == Sky) {
        const std::string spec = stage + "\\skyspec.txt";
        std::ofstream out(spec, std::ios::binary | std::ios::trunc);
        if (out) { out << "skyshader = sky_basic.fx\r\n\r\n"; out.flush(); }
        if (!out.good()) { error = "Failed to create Classic sky specification."; ok = false; }
        else stagedFiles.push_back("skyspec.txt");
    }
    if (ok) {
        const std::string settings = stage + "\\pw_style_settings.txt";
        std::ofstream out(settings, std::ios::binary | std::ios::trunc);
        if (out) { out << "scale=" << sizePercent << "\nrotation=" << yawDeg << "\n"; out.flush(); }
        if (!out.good()) { error = "Failed to write environment model settings."; ok = false; }
        else stagedFiles.push_back("pw_style_settings.txt");
    }
    if (ok && !MoveFileA(stage.c_str(), dest.c_str())) {
        error = "Cannot publish style. Check permissions and duplicate style names."; ok = false;
    }
    if (!ok) {
        PWStyleImport::RemoveStaging(stage, stagedFiles);
        return false;
    }
    return true;
}
} // namespace PWEnvironment
