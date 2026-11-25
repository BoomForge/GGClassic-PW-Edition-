// Win32 C++14 contract tests for PW-LevelFlow and PW-FPMCache.
// Test only synthetic data, never real GameGuru content or level archives.
#include "PW-LevelFlow.h"
#include "PW-FPMCache.h"
#include "PW-PlayerProfile.h"
#include "PW-DisplayOptions.h"
#include "PW-GameplayControls.h"
#include "PW-ThirdPerson.h"
#include "PW-ThirdPersonGameplay.h"
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

    // Independent profile storage, atomic replace, recovery and path rules.
    const std::string profileRoot=PWProfile::Root(root,true);
    assert(!profileRoot.empty());
    assert(!PWProfile::Name("../escape"));
    assert(!PWProfile::Name("COM1"));
    assert(!PWProfile::Name("nul"));
    PWProfile::Snapshot first; first.level="Interiors\\cave.fpm";first.payload="PWGAME1\nfirst\n";
    assert(PWProfile::Save(profileRoot,"Hero",first));
    PWProfile::Snapshot second=first;second.payload="PWGAME1\nsecond\n";
    assert(PWProfile::Save(profileRoot,"hero",second));
    PWProfile::Snapshot loaded;
    assert(PWProfile::Load(profileRoot,"HERO",loaded)&&loaded.payload==second.payload&&!loaded.recovered);
    assert(PWProfile::List(profileRoot).size()==1);
    Put(PWProfile::Path(profileRoot,"hero"),"corrupt");
    assert(PWProfile::Load(profileRoot,"hero",loaded)&&loaded.payload==first.payload&&loaded.recovered);
    assert(PWProfile::Save(profileRoot,"hero",second));
    Put(PWProfile::Path(profileRoot,"hero"),"corrupt again");
    assert(PWProfile::Load(profileRoot,"hero",loaded)&&loaded.payload==first.payload&&loaded.recovered);
    assert(!PWProfile::Load(profileRoot,"missing",loaded));
    assert(!PWProfile::Save(profileRoot,"../escape",first));
    PWProfile::Snapshot invalid=first;invalid.level="..\\outside.fpm";
    assert(!PWProfile::Save(profileRoot,"hero",invalid));
    invalid=first;invalid.payload.assign(PWProfile::MaxPayload+1,'x');
    assert(!PWProfile::Save(profileRoot,"hero",invalid));
    std::string envelope;
    assert(PWProfile::Encode(first,envelope));
    envelope.back()='X';
    assert(!PWProfile::Decode(envelope,loaded));
    assert(PWProfile::CurrentLevel(mapbank,"mapbank\\Interiors\\cave.fpm",area));
    const std::vector<PWDisplay::Size> modes={{800,600},{1280,720}};
    const auto safe=PWDisplay::Safe(modes,1920,1080);
    assert(!safe.fullscreen&&safe.width==1280&&safe.height==720);
    assert(PWDisplay::Valid(safe,modes,1920,1080));
    auto bad=safe;bad.width=9999;
    assert(!PWDisplay::Valid(bad,modes,1920,1080));
    const auto full=PWDisplay::Desktop(1920,1080);
    assert(PWDisplay::Valid(full,modes,1920,1080));
    assert(!PWDisplay::Valid(full,modes,1280,720));
    assert(PWDisplay::Save(root,safe));
    PWDisplay::Choice saved;
    assert(PWDisplay::Read(root,saved)&&PWDisplay::Same(saved,safe));
    assert(PWDisplay::MarkPending(root)&&PWDisplay::HasPending(root));
    PWDisplay::ClearPending(root);assert(!PWDisplay::HasPending(root));
    Put(PWDisplay::SettingsPath(root),"garbage");
    assert(!PWDisplay::Read(root,saved));
    auto bindings=PWControls::Classic();
    assert(PWControls::Valid(bindings));
    bindings[PWControls::Forward]=200;
    assert(PWControls::Valid(bindings));
    assert(PWControls::Save(root,bindings));
    PWControls::Bindings restoredBindings={};
    assert(PWControls::Read(root,restoredBindings)&&restoredBindings==bindings);
    auto conflict=bindings;conflict[PWControls::Backward]=200;
    assert(!PWControls::Save(root,conflict));
    assert(PWControls::Read(root,restoredBindings)&&restoredBindings==bindings);
    conflict=bindings;conflict[PWControls::Forward]=1;
    assert(!PWControls::Valid(conflict));
    Put(PWControls::Path(root),"PWCONTROLS1\n200\n");
    assert(!PWControls::Read(root,restoredBindings)&&restoredBindings==bindings);
    assert(PWControls::Index(18)==PWControls::Interact);
    using namespace PWThirdPerson;
    const auto orbit=Build(0,0,200,20,0,false);
    assert(std::fabs(orbit.offset.x-20)<0.001f&&std::fabs(orbit.offset.z+200)<0.001f);
    const auto sideways=Build(90,0,200,0,0,false);
    assert(std::fabs(sideways.offset.x+200)<0.001f);
    assert(std::fabs(Facing(0,1,0,0,1,0)-45)<0.001f);
    assert(Facing(0,1,1,1,1,90)==90);
    assert(std::fabs(ContactFraction({0,0,0},{0,0,100},{0,0,50},5)-0.45f)<0.001f);
    assert(Boom(1,0.2f,0.016f,8)==0.2f);
    assert(Boom(0.2f,1,0.016f,8)>0.2f&&Boom(0.2f,1,0.016f,8)<1);
    assert(ApproachAngle(359,1,0.1f)>359||ApproachAngle(359,1,0.1f)<1);
    assert(InReach({0,0,0},{0,0,100},100));
    assert(!InReach({0,0,0},{0,0,101},100));
    assert(!InReach({0,0,0},{0,80,80},100));
    assert(InMeleeArc({0,0,0},{0,0,30},359,50,10,20));
    assert(!InMeleeArc({0,0,0},{0,0,-30},0,50,45,20));
    assert(!InMeleeArc({0,0,0},{0,30,20},0,50,45,20));
    assert(StrikeDamage(10,10,0)==10&&StrikeDamage(10,10,999)==10);
    assert(StrikeDamage(20,10,0)==10&&StrikeDamage(20,10,10)==20);
    assert(StrikeDamage(-10,-1,123)==0);
    assert(EquipAllowed(0,100,false,false));
    assert(EquipAllowed(4,100,true,true));
    assert(!EquipAllowed(4,100,false,true));
    assert(!EquipAllowed(4,100,true,false));
    assert(!EquipAllowed(-1,100,true,true));
    assert(!EquipAllowed(101,100,true,true));
    printf("PW level flow, profiles, display, controls and native third-person math checks passed.\n");
    return 0;
}
