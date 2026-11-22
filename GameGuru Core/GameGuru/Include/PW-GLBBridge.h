#pragma once
// Pixelwolf GGC Model Importer 2: GLB 2.0 input gate and explicit Blender bridge.
// GLB -> temporary FBX -> existing LoadFBX pipeline. No native glTF renderer,
// retargeter or Blender binaries are bundled with PW Edition.
#include "PW-ImportPaths.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <vector>
#endif

namespace PWGLB {
inline std::uint32_t U32(const unsigned char* p) {
    return (std::uint32_t)p[0] | ((std::uint32_t)p[1] << 8)
        | ((std::uint32_t)p[2] << 16) | ((std::uint32_t)p[3] << 24);
}
inline bool IsGLB(const std::string& path) {
    return PWImport::EndsWithI(path.c_str(), ".glb");
}
inline bool Validate(const std::string& path, std::string& problem) {
    problem.clear();
    if (!IsGLB(path)) { problem = "Select a binary glTF 2.0 .glb file."; return false; }
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.good()) { problem = "GLB file could not be opened."; return false; }
    const std::streamoff size = in.tellg();
    // Bound memory / conversion time. This header check does not parse meshes.
    if (size < 24 || size > 512LL * 1024 * 1024) {
        problem = "GLB file has invalid size (max 512 MB)."; return false;
    }
    in.seekg(0);
    unsigned char head[24] = {};
    in.read(reinterpret_cast<char*>(head), sizeof(head));
    if (in.gcount() != sizeof(head) || head[0] != 'g' || head[1] != 'l'
        || head[2] != 'T' || head[3] != 'F' || U32(head + 4) != 2) {
        problem = "File is not a glTF 2.0 GLB container."; return false;
    }
    if (U32(head + 8) != (std::uint64_t)size) {
        problem = "GLB declared length does not match the file."; return false;
    }
    const std::uint32_t jsonLength = U32(head + 12);
    if (U32(head + 16) != 0x4E4F534A || jsonLength < 2
        || (jsonLength % 4) != 0 || jsonLength > (std::uint64_t)size - 20) {
        problem = "GLB first chunk is not valid JSON."; return false;
    }
    // A glTF GLB starts with a JSON object (possibly after whitespace).
    bool jsonBeginsWithObject = false;
    for (std::uint32_t i = 0; i < (std::min)(jsonLength, (std::uint32_t)32); ++i) {
        const unsigned char b = i < 4 ? head[20 + i] : (unsigned char)in.get();
        if (b == ' ' || b == '\t' || b == '\r' || b == '\n') continue;
        jsonBeginsWithObject = b == '{';
        break;
    }
    if (!jsonBeginsWithObject) { problem = "GLB JSON does not begin with an object."; return false; }
    return true;
}
#ifdef _WIN32
inline std::wstring Wide(const std::string& value) {
    if (value.empty()) return std::wstring();
    // Classic's filename picker supplies paths using the process code page.
    const int n = MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, nullptr, 0);
    if (n < 1) return std::wstring();
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, &w[0], n);
    w.resize(n - 1);
    return w;
}
inline std::string Narrow(const std::wstring& value) {
    if (value.empty()) return std::string();
    const int n = WideCharToMultiByte(CP_ACP, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n < 1) return std::string();
    std::string s(n, '\0');
    WideCharToMultiByte(CP_ACP, 0, value.c_str(), -1, &s[0], n, nullptr, nullptr);
    s.resize(n - 1);
    return s;
}
inline bool Exists(const std::wstring& path, bool folder = false) {
    const DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !!(a & FILE_ATTRIBUTE_DIRECTORY) == folder;
}
inline std::wstring Quote(const std::wstring& value) {
    // Windows CreateProcess quoting; handles spaces, literal quotes and backslashes.
    std::wstring out = L"\"";
    unsigned int slashes = 0;
    for (wchar_t c : value) {
        if (c == L'\\') { ++slashes; continue; }
        if (c == L'"') {
            out.append(slashes * 2 + 1, L'\\'); out += c; slashes = 0;
            continue;
        }
        out.append(slashes, L'\\'); slashes = 0; out += c;
    }
    out.append(slashes * 2, L'\\');
    out += L'"';
    return out;
}
inline std::wstring Blender() {
    wchar_t env[32768] = {};
    const DWORD n = GetEnvironmentVariableW(L"PW_GLB_BLENDER_EXE", env, 32768);
    if (n > 0 && n < 32768 && Exists(env)) return env;
    wchar_t found[32768] = {};
    if (SearchPathW(nullptr, L"blender.exe", nullptr, 32768, found, nullptr)
        && Exists(found)) return found;
    // Blender's regular Windows install layout when it isn't on PATH.
    wchar_t programFiles[32768] = {};
    if (GetEnvironmentVariableW(L"ProgramFiles", programFiles, 32768) == 0)
        return std::wstring();
    const std::wstring parent = std::wstring(programFiles) + L"\\Blender Foundation\\";
    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileW((parent + L"Blender *").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) return std::wstring();
    std::wstring best;
    do {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        const std::wstring versionFolder = data.cFileName;
        const std::wstring candidate = parent + versionFolder + L"\\blender.exe";
        if (Exists(candidate) && (best.empty() || candidate > best)) best = candidate;
    } while (FindNextFileW(find, &data));
    FindClose(find);
    return best;
}
inline bool Convert(const std::string& source, const std::string& gameFilesDirectory,
                    std::string& outputFBX, std::string& problem) {
    outputFBX.clear();
    if (!Validate(source, problem)) return false;
    const std::wstring blender = Blender();
    if (blender.empty()) {
        problem = "Blender is required for GLB import. Install Blender or set PW_GLB_BLENDER_EXE.";
        return false;
    }
    std::wstring root = Wide(gameFilesDirectory);
    if (root.empty()) { problem = "GameGuru Files directory is not valid."; return false; }
    if (root.back() != L'\\' && root.back() != L'/') root += L'\\';
    const std::wstring script = root + L"PWTools\\pw_glb_to_fbx.py";
    if (!Exists(script)) {
        problem = "PW GLB converter script missing from Files\\PWTools; reinstall PW Edition.";
        return false;
    }
    const std::wstring importerFolder = root + L"importer\\";
    const std::wstring temp = importerFolder + L"temp\\";
    CreateDirectoryW(importerFolder.c_str(), nullptr);
    CreateDirectoryW(temp.c_str(), nullptr);
    if (!Exists(temp, true)) { problem = "GameGuru importer temporary directory is not writable."; return false; }
    // Per-import directory prevents stale files/materials overwriting earlier
    // imports or one conversion colliding with another in the same process.
    const std::wstring unique = L"pw_glb_" + std::to_wstring(GetCurrentProcessId())
        + L"_" + std::to_wstring(GetTickCount64());
    const std::wstring working = temp + unique + L"\\";
    if (!CreateDirectoryW(working.c_str(), nullptr)) {
        problem = "Unable to make an isolated GLB conversion folder."; return false;
    }
    const std::wstring output = working + L"model.fbx";
    const std::wstring logfile = working + L"conversion.log";
    std::wstring command = Quote(blender) + L" --background --factory-startup"
        + L" --python " + Quote(script) + L" -- " + Quote(Wide(source))
        + L" " + Quote(output) + L" " + Quote(logfile);
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(0);
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(blender.c_str(), mutableCommand.data(), nullptr, nullptr,
                        FALSE, CREATE_NO_WINDOW, nullptr, working.c_str(),
                        &startup, &process)) {
        problem = "Blender could not be launched for GLB conversion."; return false;
    }
    const DWORD wait = WaitForSingleObject(process.hProcess, 180000);
    if (wait == WAIT_TIMEOUT) {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 5000);
    }
    DWORD exitCode = 1;
    if (wait == WAIT_OBJECT_0) GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (wait != WAIT_OBJECT_0 || exitCode != 0 || !Exists(output)) {
        problem = wait == WAIT_TIMEOUT ? "GLB conversion timed out."
            : "GLB conversion failed. See importer temp pw_glb_*\\conversion.log.";
        return false;
    }
    WIN32_FILE_ATTRIBUTE_DATA attributes = {};
    if (!GetFileAttributesExW(output.c_str(), GetFileExInfoStandard, &attributes)
        || (attributes.nFileSizeHigh == 0 && attributes.nFileSizeLow < 64)) {
        problem = "Blender created an empty or invalid FBX file."; return false;
    }
    outputFBX = Narrow(output);
    return !outputFBX.empty();
}
#endif
} // namespace PWGLB
