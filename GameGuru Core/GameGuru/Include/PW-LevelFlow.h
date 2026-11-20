#pragma once
// Pixelwolf Edition: conservative single-player FPM destination validation,
// repeat-lookup caching and optional asynchronous archive warm-up.
// No GameGuru types: may be built and tested independently on Windows x64.
#ifdef _WIN32
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <map>
#include <mutex>
#include <new>
#include <string>

namespace PWLevelFlow
{
    struct Destination
    {
        std::string relative; // relative to Files\mapbank, retains .fpm
        std::string absolute; // full FPM path
    };

    inline std::string Lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    inline bool SafeRequestedName(const std::string& name)
    {
        if (name.empty() || name.size() > 230) return false;
        if (name[0] == '\\' || name[0] == '/') return false;
        if (name.find(':') != std::string::npos) return false;
        std::string part;
        for (size_t n = 0; n <= name.size(); ++n)
        {
            const char c = n < name.size() ? name[n] : '\\';
            if (c == '\\' || c == '/')
            {
                if (part.empty() || part == "." || part == ".." ||
                    part.back() == '.' || part.back() == ' ')
                    return false;
                part.clear();
            }
            else
            {
                const unsigned char uc = static_cast<unsigned char>(c);
                if (uc < 32 || c == '<' || c == '>' || c == '"' || c == '|' ||
                    c == '*' || c == '?' ) return false;
                part += c;
            }
        }
        return true;
    }

    inline std::string Normalise(std::string name)
    {
        std::replace(name.begin(), name.end(), '/', '\\');
        if (name.size() < 4 || Lower(name.substr(name.size() - 4)) != ".fpm")
            name += ".fpm";
        return name;
    }

    inline std::string Root(std::string value)
    {
        std::replace(value.begin(), value.end(), '/', '\\');
        if (!value.empty() && value.back() != '\\') value += '\\';
        return value;
    }

    inline bool IsFile(const std::string& path)
    {
        WIN32_FILE_ATTRIBUTE_DATA data;
        return GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data) &&
            (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
            (data.nFileSizeHigh != 0 || data.nFileSizeLow != 0);
    }

    // No negative caching: an asset added to a running installation is found.
    inline bool Resolve(const std::string& mapbank, const std::string& request, Destination& out)
    {
        if (!SafeRequestedName(request)) return false;
        const std::string base = Root(mapbank);
        if (base.empty() || base.size() >= MAX_PATH - 240) return false;
        const std::string level = Normalise(request);
        if (!SafeRequestedName(level)) return false;

        static std::map<std::string, std::string> previous;
        static std::mutex cacheMutex;
        const std::string key = Lower(base + level);
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            const std::map<std::string, std::string>::iterator cached = previous.find(key);
            if (cached != previous.end())
            {
                if (IsFile(base + cached->second))
                {
                    out.relative = cached->second;
                    out.absolute = base + out.relative;
                    return true;
                }
                previous.erase(cached);
            }
        }

        std::string found;
        if (IsFile(base + level))
        {
            found = level;
        }
        else if (level.find('\\') == std::string::npos)
        {
            // Compatibility with Classic's existing one-subfolder search.
            WIN32_FIND_DATAA data = {};
            HANDLE find = FindFirstFileA((base + "*").c_str(), &data);
            if (find != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
                        (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                        continue;
                    const std::string folder = data.cFileName;
                    if (folder == "." || folder == ".." || !SafeRequestedName(folder))
                        continue;
                    const std::string trial = folder + "\\" + level;
                    if (IsFile(base + trial)) { found = trial; break; }
                } while (FindNextFileA(find, &data));
                FindClose(find);
            }
        }

        if (found.empty()) return false;
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            previous[key] = found;
        }
        out.relative = found;
        out.absolute = base + found;
        return true;
    }

    inline bool PrefetchEnabled()
    {
        char value[4] = {};
        return GetEnvironmentVariableA("GGCPW_PREFETCH_LEVELS", value, 4) == 1 && value[0] == '1';
    }

    inline DWORD WINAPI PrefetchWorker(LPVOID value)
    {
        std::string* fileName = static_cast<std::string*>(value);
        if (fileName == NULL) return 0;
        HANDLE file = CreateFileA(fileName->c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        delete fileName;
        if (file != INVALID_HANDLE_VALUE)
        {
            char buffer[64 * 1024];
            DWORD read = 0;
            // Bounded warming: hints Windows' cache without holding level data in RAM.
            unsigned long long total = 0;
            while (total < 64ULL * 1024ULL * 1024ULL &&
                ReadFile(file, buffer, sizeof(buffer), &read, NULL) && read > 0)
            {
                total += read;
            }
            CloseHandle(file);
        }
        return 0;
    }

    inline bool Prefetch(const Destination& target)
    {
        if (!PrefetchEnabled() || !IsFile(target.absolute)) return false;
        std::string* owned = new(std::nothrow) std::string(target.absolute);
        if (!owned) return false;
        HANDLE thread = CreateThread(NULL, 0, PrefetchWorker, owned,
            CREATE_SUSPENDED, NULL);
        if (thread == NULL) { delete owned; return false; }
        // Don't let a busy disk prefetch block the main game's transition fade.
        SetThreadPriority(thread, THREAD_PRIORITY_BELOW_NORMAL);
        ResumeThread(thread);
        CloseHandle(thread); // worker owns only its string; no dangling engine references
        return true;
    }
}
#endif
