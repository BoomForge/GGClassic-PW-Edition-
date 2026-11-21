// Win32 C++14 contract tests for PW-LevelFlow and PW-FPMCache.
// Test only synthetic data, never real GameGuru content or level archives.
#include "PW-LevelFlow.h"
#include "PW-FPMCache.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <windows.h>

static void Put(const std::string& path, const std::string& content)
{
    HANDLE h = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(h != INVALID_HANDLE_VALUE);
    DWORD written = 0;
    assert(WriteFile(h, content.data(), (DWORD)content.size(), &written, NULL));
    assert(written == content.size());
    CloseHandle(h);
}

static std::string Read(const std::string& path)
{
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(h != INVALID_HANDLE_VALUE);
    char buf[128] = {};
    DWORD n = 0;
    assert(ReadFile(h, buf, sizeof(buf), &n, NULL));
    CloseHandle(h);
    return std::string(buf, n);
}

int main()
{
    char temp[MAX_PATH] = {};
    assert(GetTempPathA(MAX_PATH, temp) > 0);
    const std::string root = std::string(temp) + "PWLevelFlowSmoke-" + std::to_string(GetCurrentProcessId()) + "\\";
    assert(CreateDirectoryA(root.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
    const std::string mapbank = root + "mapbank\\";
    const std::string interiors = mapbank + "Interiors\\";
    const std::string extracted = root + "testmap\\";
    const std::string restored = root + "restored\\";
    assert(CreateDirectoryA(mapbank.c_str(), NULL));
    assert(CreateDirectoryA(interiors.c_str(), NULL));
    assert(CreateDirectoryA(extracted.c_str(), NULL));
    assert(CreateDirectoryA(restored.c_str(), NULL));
    Put(interiors + "cave.fpm", "synthetic-level-archive");

    PWLevelFlow::Destination area;
    assert(PWLevelFlow::Resolve(mapbank, "cave", area));
    assert(area.relative == "Interiors\\cave.fpm");
    assert(area.absolute == interiors + "cave.fpm");
    // Cached lookup and explicit subfolder names.
    assert(PWLevelFlow::Resolve(mapbank, "cave", area));
    assert(PWLevelFlow::Resolve(mapbank, "Interiors/cave.fpm", area));
    // A newly created direct FPM overrides a previously cached nested match.
    Put(mapbank + "cave.fpm", "synthetic-root-level");
    assert(PWLevelFlow::Resolve(mapbank, "cave", area));
    assert(area.relative == "cave.fpm");
    assert(DeleteFileA((mapbank + "cave.fpm").c_str()));
    assert(PWLevelFlow::Resolve(mapbank, "cave", area));
    assert(area.relative == "Interiors\\cave.fpm");
    assert(!PWLevelFlow::Resolve(mapbank, "../cave", area));
    assert(!PWLevelFlow::Resolve(mapbank, "C:\\Windows\\exploit.fpm", area));
    assert(!PWLevelFlow::Resolve(mapbank, "not-here", area));
    assert(!PWLevelFlow::Resolve(mapbank, "\\evil", area));

    // Missing cached file never results in a stale positive lookup.
    assert(DeleteFileA((interiors + "cave.fpm").c_str()));
    assert(!PWLevelFlow::Resolve(mapbank, "cave", area));
    Put(interiors + "cave.fpm", "synthetic-level-archive");

    SetEnvironmentVariableA("GGCPW_PREFETCH_LEVELS", "1");
    assert(PWLevelFlow::Resolve(mapbank, "cave", area));
    assert(PWLevelFlow::Prefetch(area)); // bounded background read
    Sleep(50);
    SetEnvironmentVariableA("GGCPW_PREFETCH_LEVELS", NULL);
    assert(!PWLevelFlow::Prefetch(area));

    // Extraction cache is OFF by default.
    SetEnvironmentVariableA("GGCPW_FPM_CACHE", NULL);
    assert(!PWFPMCache::Enabled());
    SetEnvironmentVariableA("GGCPW_FPM_CACHE", "1");
    assert(PWFPMCache::Enabled());

    Put(extracted + "header.dat", "synthetic-header");
    Put(extracted + "visuals.ini", "synthetic-visuals");
    const std::vector<std::string> items = {"header.dat", "visuals.ini"};
    assert(PWFPMCache::Remember(interiors + "cave.fpm", extracted, items));
    assert(PWFPMCache::Restore(interiors + "cave.fpm", restored));
    assert(Read(restored + "header.dat") == "synthetic-header");
    assert(Read(restored + "visuals.ini") == "synthetic-visuals");

    // A corrupted cache member must NOT be restored, even for an unchanged archive.
    const std::string cachedHeader = PWFPMCache::CacheRoot(interiors + "cave.fpm") + "header.dat";
    Put(cachedHeader, "corrupted-cache-data");
    assert(!PWFPMCache::Restore(interiors + "cave.fpm", restored));
    assert(PWFPMCache::Remember(interiors + "cave.fpm", extracted, items));
    assert(PWFPMCache::Restore(interiors + "cave.fpm", restored));

    // Changed FPM must invalidate the snapshot instead of returning stale assets.
    Put(interiors + "cave.fpm", "synthetic-level-archive-modified");
    assert(!PWFPMCache::Restore(interiors + "cave.fpm", restored));
    assert(!PWFPMCache::SafeRelative("../header.dat"));
    assert(!PWFPMCache::SafeRelative("C:\\injected.dat"));
    assert(!PWFPMCache::SafeRelative("foo\\..\\bar.dat"));
    SetEnvironmentVariableA("GGCPW_FPM_CACHE", NULL);

    printf("PW LevelFlow + FPM cache contract checks passed.\n");
    return 0;
}
