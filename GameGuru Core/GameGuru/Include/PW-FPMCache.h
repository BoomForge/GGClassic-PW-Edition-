#pragma once
// Pixelwolf Edition: opt-in reusable extraction cache for standalone .FPM levels.
// Caches only data extracted from a local archive, keyed by its size/mtime.
// Does not cache live game objects or modify original .fpm files.
// Default OFF until complete GameGuru runtime validation.
#ifdef _WIN32
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace PWFPMCache
{
    struct Identity
    {
        DWORD sizeLow = 0;
        DWORD sizeHigh = 0;
        FILETIME modified = {};
    };

    inline bool Stat(const std::string& path, Identity& id)
    {
        WIN32_FILE_ATTRIBUTE_DATA data = {};
        if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data) ||
            (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            return false;
        id.sizeLow = data.nFileSizeLow;
        id.sizeHigh = data.nFileSizeHigh;
        id.modified = data.ftLastWriteTime;
        return true;
    }

    inline bool Equal(const Identity& a, const Identity& b)
    {
        return a.sizeHigh == b.sizeHigh && a.sizeLow == b.sizeLow &&
            CompareFileTime(&a.modified, &b.modified) == 0;
    }

    inline bool Enabled()
    {
        char value[4] = {};
        return GetEnvironmentVariableA("GGCPW_FPM_CACHE", value, 4) == 1 && value[0] == '1';
    }

    inline std::string Lower(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return str;
    }

    inline std::string Root(std::string dir)
    {
        std::replace(dir.begin(), dir.end(), '/', '\\');
        if (!dir.empty() && dir.back() != '\\') dir.push_back('\\');
        return dir;
    }

    inline bool SafeRelative(std::string name)
    {
        if (name.empty() || name.size() > 220) return false;
        std::replace(name.begin(), name.end(), '/', '\\');
        if (name[0] == '\\' || name.find(':') != std::string::npos) return false;
        std::string part;
        for (size_t i = 0; i <= name.size(); ++i)
        {
            char ch = i < name.size() ? name[i] : '\\';
            if (ch == '\\')
            {
                if (part.empty() || part == "." || part == ".." || part.back() == '.' ||
                    part.back() == ' ')
                    return false;
                part.clear();
            }
            else
            {
                unsigned char c = static_cast<unsigned char>(ch);
                if (c < 32 || ch == '|' || ch == '<' || ch == '>' ||
                    ch == '"' || ch == '*' || ch == '?')
                    return false;
                part.push_back(ch);
            }
        }
        return true;
    }

    inline bool EnsureDirectories(const std::string& absoluteFile)
    {
        if (absoluteFile.size() >= MAX_PATH) return false;
        for (size_t i = 3; i < absoluteFile.size(); ++i)
        {
            if (absoluteFile[i] == '\\' || absoluteFile[i] == '/')
            {
                const std::string part = absoluteFile.substr(0, i);
                if (!CreateDirectoryA(part.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
                    return false;
            }
        }
        return true;
    }

    inline unsigned long long Hash(const std::string& name)
    {
        unsigned long long h = 14695981039346656037ULL;
        for (unsigned char c : name)
        {
            h ^= c;
            h *= 1099511628211ULL;
        }
        return h;
    }

    inline std::string CacheRoot(const std::string& archive)
    {
        char temp[MAX_PATH] = {};
        DWORD n = GetTempPathA(MAX_PATH, temp);
        if (n == 0 || n >= MAX_PATH) return "";
        char folder[90] = {};
        sprintf_s(folder, sizeof(folder), "GGCPW-FPMCache-%lu\\%016llx\\",
            (unsigned long)GetCurrentProcessId(), Hash(Lower(archive)));
        return Root(temp) + folder;
    }

    struct Entry
    {
        std::string name;
        Identity identity;
    };

    struct Snapshot
    {
        Identity source;
        std::string cacheRoot;
        std::vector<Entry> entries;
    };

    inline std::map<std::string, Snapshot>& Snapshots()
    {
        static std::map<std::string, Snapshot> data;
        return data;
    }

    inline std::mutex& Lock()
    {
        static std::mutex guard;
        return guard;
    }

    // On a cache miss (or any inconsistency), caller must extract the original FPM.
    inline bool Restore(const std::string& sourceArchive, const std::string& targetDir)
    {
        if (!Enabled()) return false;
        Identity source = {};
        if (!Stat(sourceArchive, source)) return false;
        Snapshot shot;
        {
            std::lock_guard<std::mutex> guard(Lock());
            const auto it = Snapshots().find(Lower(sourceArchive));
            if (it == Snapshots().end() || !Equal(it->second.source, source))
                return false;
            shot = it->second;
        }
        if (shot.entries.empty()) return false;

        // Validate ALL cache members before copying anything into live levelbank\testmap.
        for (const Entry& e : shot.entries)
        {
            Identity cached = {};
            if (!SafeRelative(e.name) ||
                !Stat(shot.cacheRoot + e.name, cached) ||
                !Equal(e.identity, cached))
                return false;
        }

        const std::string destRoot = Root(targetDir);
        if (destRoot.empty()) return false;
        for (const Entry& e : shot.entries)
        {
            const std::string target = destRoot + e.name;
            if (!EnsureDirectories(target) ||
                !CopyFileA((shot.cacheRoot + e.name).c_str(), target.c_str(), FALSE))
                return false; // fall back to complete original archive extraction
        }
        return true;
    }

    // After a successful original extraction, snapshot immutable archive contents.
    // Skipped if disk space/access is insufficient; NEVER blocks standard level loading.
    inline bool Remember(const std::string& sourceArchive, const std::string& extractedRoot,
        const std::vector<std::string>& archiveFiles)
    {
        if (!Enabled() || archiveFiles.empty()) return false;
        Identity source = {};
        if (!Stat(sourceArchive, source)) return false;

        const std::string cache = CacheRoot(sourceArchive);
        if (cache.empty() || cache.size() >= MAX_PATH - 2) return false;
        if (!EnsureDirectories(cache + "placeholder")) return false;

        Snapshot shot;
        shot.source = source;
        shot.cacheRoot = cache;
        const std::string root = Root(extractedRoot);
        if (root.empty()) return false;
        for (const std::string& raw : archiveFiles)
        {
            std::string name = raw;
            std::replace(name.begin(), name.end(), '/', '\\');
            if (!SafeRelative(name)) return false;
            Identity original = {};
            if (!Stat(root + name, original)) return false;
            const std::string target = cache + name;
            if (!EnsureDirectories(target) || !CopyFileA((root + name).c_str(), target.c_str(), FALSE))
                return false;
            Identity cached = {};
            if (!Stat(target, cached)) return false;
            shot.entries.push_back({name, cached});
        }
        {
            std::lock_guard<std::mutex> guard(Lock());
            Snapshots()[Lower(sourceArchive)] = shot;
        }
        return true;
    }
}
#endif
