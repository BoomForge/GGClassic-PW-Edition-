#pragma once
// PW Edition: safely install existing Classic skybank/vegbank style folders.
// Source is chosen with a normal file dialog (skyspec.txt or veg.DBO/veg.X).
// No material/mesh format conversion, destructive overwrite, or arbitrary code.
#include "PW-ImportPaths.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace PWStyleImport {
enum class Kind { Sky, Grass };
inline bool Same(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    return true;
}
inline bool SafeName(const std::string& name) {
    if (name.empty() || name.size() > 70 || name == "." || name == "..") return false;
    for (char c : name) {
        const unsigned char b = static_cast<unsigned char>(c);
        if (!std::isalnum(b) && c != ' ' && c != '-' && c != '_') return false;
    }
    return true;
}
inline bool AllowedAsset(const std::string& path) {
    const char* extensions[] = { ".dds", ".png", ".jpg", ".jpeg", ".bmp", ".tga",
                                  ".dbo", ".x", ".txt", ".fx" };
    for (const char* ext : extensions)
        if (PWImport::EndsWithI(path.c_str(), ext)) return true;
    return false;
}
#ifdef _WIN32
inline bool RegularFile(const std::string& file) {
    DWORD attrs = GetFileAttributesA(file.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}
inline bool Directory(const std::string& dir) {
    DWORD attrs = GetFileAttributesA(dir.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)
           && !(attrs & FILE_ATTRIBUTE_REPARSE_POINT);
}
inline bool CanInstall(const std::string& marker, const std::string& sourceDir,
                       const std::string& style, Kind kind, std::string& error) {
    if (!SafeName(style) || !Directory(sourceDir)) {
        error = "Style folder name must use letters, numbers, spaces, hyphens or underscores.";
        return false;
    }
    const std::string leaf = PWImport::BaseName(marker);
    if (kind == Kind::Sky) {
        if (!Same(leaf, "skyspec.txt") || !RegularFile(sourceDir + "\\skyspec.txt") ||
            (!RegularFile(sourceDir + "\\" + style + ".dbo") &&
             !RegularFile(sourceDir + "\\" + style + ".x"))) {
            error = "Choose skyspec.txt inside a Classic sky style folder containing the matching <folder>.DBO or .X model.";
            return false;
        }
    } else {
        if ((!Same(leaf, "veg.dbo") && !Same(leaf, "veg.x")) ||
            !RegularFile(marker) ||
            (!RegularFile(sourceDir + "\\grass.dds") &&
             !RegularFile(sourceDir + "\\grass.png"))) {
            error = "Choose veg.DBO or veg.X inside a vegetation style folder with grass.dds or grass.png.";
            return false;
        }
    }
    return true;
}
inline void RemoveStaging(const std::string& dir, const std::vector<std::string>& copied) {
    for (const std::string& name : copied)
        DeleteFileA((dir + "\\" + name).c_str());
    RemoveDirectoryA(dir.c_str());
}
inline bool Install(const std::string& marker, const std::string& bank,
                    Kind kind, std::string& style, std::string& error) {
    style.clear();
    error.clear();
    std::string sourceDir = PWImport::DirectoryOf(marker);
    while (!sourceDir.empty() && (sourceDir.back() == '\\' || sourceDir.back() == '/'))
        sourceDir.pop_back();
    const std::string proposed = PWImport::BaseName(sourceDir);
    if (!CanInstall(marker, sourceDir, proposed, kind, error)) return false;
    if (!Directory(bank)) { error = "GameGuru destination bank folder is missing."; return false; }
    const std::string finalDir = bank + "\\" + proposed;
    if (GetFileAttributesA(finalDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
        error = "A style with this folder name already exists. Rename the source folder first.";
        return false;
    }
    const std::string staging = bank + "\\pw_import_" +
        std::to_string(GetCurrentProcessId()) + "_" + std::to_string(GetTickCount64());
    if (staging.size() >= MAX_PATH - 88 || sourceDir.size() >= MAX_PATH - 88) {
        error = "Style path is too long for GameGuru Classic."; return false;
    }
    if (!CreateDirectoryA(staging.c_str(), nullptr)) {
        error = "Cannot create staging folder in the selected GameGuru bank."; return false;
    }
    std::vector<std::string> copied;
    std::uint64_t bytes = 0;
    bool ok = true;
    WIN32_FIND_DATAA data = {};
    HANDLE find = FindFirstFileA((sourceDir + "\\*").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        error = "Cannot enumerate source style folder.";
        RemoveStaging(staging, copied);
        return false;
    }
    do {
        const std::string name = data.cFileName;
        if (name == "." || name == "..") continue;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            error = "Style folder contains a link/reparse point; import a regular folder.";
            ok = false; break;
        }
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            error = "This importer currently accepts flat Classic style folders, not nested subfolders.";
            ok = false; break;
        }
        if (!AllowedAsset(name)) {
            error = "Unsupported file type in style folder: " + name;
            ok = false; break;
        }
        const std::uint64_t size = (std::uint64_t(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
        bytes += size;
        if (copied.size() >= 256 || bytes > 512ULL * 1024 * 1024) {
            error = "Style exceeds 256 files or 512 MB import limit."; ok = false; break;
        }
        if (!CopyFileA((sourceDir + "\\" + name).c_str(),
                       (staging + "\\" + name).c_str(), TRUE)) {
            error = "Could not copy style file: " + name;
            ok = false; break;
        }
        copied.push_back(name);
    } while (FindNextFileA(find, &data));
    FindClose(find);
    if (ok && copied.empty()) { error = "Source style folder is empty."; ok = false; }
    if (ok && !MoveFileA(staging.c_str(), finalDir.c_str())) {
        error = "Could not publish style folder; another style may already exist.";
        ok = false;
    }
    if (!ok) { RemoveStaging(staging, copied); return false; }
    style = proposed;
    return true;
}
#endif
} // namespace PWStyleImport
