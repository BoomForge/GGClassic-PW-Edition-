// Windows CI smoke check: compiles the production Lua Studio header without the full engine.
#include "PW-LuaStudio.h"
#include <cstdio>
#include <fstream>
#include <string>

int wmain() {
    wchar_t base[MAX_PATH] = {};
    DWORD chars = GetTempPathW(MAX_PATH, base);
    if (!chars || chars >= MAX_PATH) return 10;
    std::wstring directory(base);
    directory += L"pw-lua-ci-";
    directory += std::to_wstring(GetCurrentProcessId());
    if (!CreateDirectoryW(directory.c_str(), NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) return 11;
    std::wstring prefix = directory + L"\\";
    std::wstring script = prefix + L"global.lua";
    {
        std::ofstream output(script.c_str(), std::ios::binary);
        output << "function Prompt(message)\nend\n"
                  "function JumpToLevel(level)\nend\n"
                  "function PWSetTransitionValue(key, value)\nend\n";
        if (!output) return 12;
    }
    PWLuaStudio::Data().scriptbank = prefix;
    PWLuaStudio::Discover();
    auto& state = PWLuaStudio::Data();
    if (state.commands.size() != 3) return 13;
    if (state.known.count(L"Prompt") != 1) return 14;
    if (state.known.count(L"JumpToLevel") != 1) return 15;
    if (!PWLuaStudio::Key(L"function") || PWLuaStudio::Key(L"invalid")) return 16;
    if (PWLuaStudio::Utf8("ok") != L"ok") return 17;
    if (PWLuaStudio::Utf8("\xFF") != L"") return 18;
    if (PWLuaStudio::Encode(L"ok") != "ok") return 19;
    DeleteFileW(script.c_str());
    RemoveDirectoryW(directory.c_str());
    puts("Lua Studio compile and command-catalog test passed");
    return 0;
}
