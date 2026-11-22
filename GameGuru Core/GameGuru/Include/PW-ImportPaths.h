#pragma once
// PW Edition: safe helpers for Blender/Mixamo FBX texture name resolution.
// Keeps the original reference first; falls back to same-folder basename,
// DDS, PNG, TGA and JPEG companions without modifying model skeletons.
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

namespace PWImport
{
    inline bool EndsWithI(const char* value, const char* suffix)
    {
        if (!value || !suffix) return false;
        const size_t n = std::strlen(value), m = std::strlen(suffix);
        if (n < m) return false;
        for (size_t i = 0; i < m; ++i)
        {
            const unsigned char a = static_cast<unsigned char>(value[n - m + i]);
            const unsigned char b = static_cast<unsigned char>(suffix[i]);
            if (std::tolower(a) != std::tolower(b)) return false;
        }
        return true;
    }

    inline std::string BaseName(const std::string& path)
    {
        const size_t slash = path.find_last_of("\\/");
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }

    inline std::string DirectoryOf(const std::string& path)
    {
        const size_t slash = path.find_last_of("\\/");
        return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
    }

    inline std::string Folder(std::string path)
    {
        if (!path.empty() && path.back() != '\\' && path.back() != '/')
            path += "\\";
        return path;
    }

    inline std::string ReplaceExtension(std::string path, const char* extension)
    {
        const size_t separator = path.find_last_of("\\/");
        const size_t dot = path.find_last_of('.');
        if (dot != std::string::npos && (separator == std::string::npos || dot > separator))
            path.resize(dot);
        return path + extension;
    }

    template <typename Exists>
    inline std::string FindTexture(const std::string& originalFolder,
                                   const std::string& reference,
                                   Exists exists)
    {
        if (reference.empty()) return "";
        // The FBX may refer to a former absolute location; honour it if present.
        if (exists(reference)) return reference;
        const std::string base = BaseName(reference);
        if (base.empty()) return "";
        const std::string folder = Folder(originalFolder);
        // Prefer the texture actually alongside the imported FBX.
        if (exists(folder + base)) return folder + base;
        // Avoid concatenating a Windows absolute path into the imported folder.
        const bool relative = reference[0] != '/' && reference[0] != '\\' &&
                              reference.find(':') == std::string::npos &&
                              reference.find("..") == std::string::npos;
        if (relative && exists(folder + reference)) return folder + reference;

        // Legacy DDS remains first when only a substitute extension exists.
        static const char* ext[] = { ".dds", ".png", ".tga", ".jpg", ".jpeg" };
        for (const char* replacement : ext)
        {
            const std::string alternate = folder + ReplaceExtension(base, replacement);
            if (exists(alternate)) return alternate;
        }
        return "";
    }
}
