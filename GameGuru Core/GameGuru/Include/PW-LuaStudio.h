// PW Edition Lua Studio - a modeless Windows RichEdit editor within Guru-MapEditor.exe.
// Native Windows control, no separately installed code editor or scripting service.
#pragma once
#include <windows.h>
#include <richedit.h>
#include <commdlg.h>
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
#pragma comment(lib, "Comdlg32.lib")

namespace PWLuaStudio {
struct Command { std::wstring name, signature, source; };
struct State {
    HWND window = NULL, editor = NULL, list = NULL, search = NULL, detail = NULL;
    HFONT font = NULL;
    HMODULE module = NULL;
    WNDPROC originalProc = NULL;
    std::wstring path, scriptbank;
    std::vector<Command> commands;
    std::set<std::wstring> known;
    bool dirty = false, suppress = false;
};
inline State& Data() { static State s; return s; }
inline bool IsWord(wchar_t c) { return iswalnum(c) != 0 || c == L'_'; }
inline std::wstring Utf8(const std::string& str) {
    if (str.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(),
        static_cast<int>(str.size()), NULL, 0);
    if (n == 0) return L"";
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(),
        static_cast<int>(str.size()), &out[0], n);
    return out;
}
inline std::string Encode(const std::wstring& str) {
    if (str.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str.data(),
        static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (n == 0) return "";
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str.data(),
        static_cast<int>(str.size()), &out[0], n, NULL, NULL);
    return out;
}
inline std::wstring Text(HWND hwnd) {
    int n = GetWindowTextLengthW(hwnd);
    std::vector<wchar_t> buf(static_cast<size_t>(n) + 1, 0);
    GetWindowTextW(hwnd, buf.data(), n + 1);
    return std::wstring(buf.data());
}
inline void Caption() {
    State& s = Data();
    std::wstring title = L"Pixelwolf Lua Studio - " +
        (s.path.empty() ? L"Untitled" : s.path) + (s.dirty ? L" *" : L"");
    if (s.window) SetWindowTextW(s.window, title.c_str());
}
inline void Status(const std::wstring& text) {
    if (Data().detail) SetWindowTextW(Data().detail, text.c_str());
}
inline std::wstring Root() {
    wchar_t path[MAX_PATH] = {};
    DWORD n = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (!n || n >= MAX_PATH) return L"";
    std::wstring full(path);
    size_t slash = full.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"" : full.substr(0, slash + 1);
}
inline void Discover() {
    State& s = Data();
    s.commands.clear(); s.known.clear();
    for (int fileId = 0; fileId < 2; ++fileId) {
        std::wstring source = fileId ? L"pw_transition_state.lua" : L"global.lua";
        std::ifstream file((s.scriptbank + source).c_str(), std::ios::binary);
        std::string line;
        while (file && std::getline(file, line)) {
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos || line.compare(start, 9, "function ") != 0) continue;
            start += 9;
            size_t open = line.find('(', start);
            size_t close = open == std::string::npos ? std::string::npos : line.find(')', open);
            if (close == std::string::npos || close - start > 200) continue;
            std::string name = line.substr(start, open - start);
            size_t trailing = name.find_last_not_of(" \t");
            if (trailing == std::string::npos) continue;
            name.resize(trailing + 1);
            if (name.empty()) continue;
            bool safe = true;
            for (char c : name) {
                if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '_' || c == '.')) safe = false;
            }
            if (!safe) continue;
            Command command;
            command.name = Utf8(name);
            command.signature = Utf8(line.substr(start, close - start + 1));
            command.source = source;
            if (s.known.insert(command.name).second) s.commands.push_back(command);
        }
    }
    std::sort(s.commands.begin(), s.commands.end(),
        [](const Command& a, const Command& b) { return a.name < b.name; });
}
inline bool Key(const std::wstring& str) {
    static const wchar_t* words[] = { L"and", L"break", L"do", L"else", L"elseif",
      L"end", L"false", L"for", L"function", L"goto", L"if", L"in", L"local",
      L"nil", L"not", L"or", L"repeat", L"return", L"then", L"true",
      L"until", L"while" };
    for (auto w : words) if (str == w) return true;
    return false;
}
inline void Color(LONG start, LONG end, COLORREF color) {
    if (start >= end) return;
    CHARRANGE range = { start, end };
    SendMessageW(Data().editor, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    CHARFORMAT2W format = {};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR;
    format.crTextColor = color;
    SendMessageW(Data().editor, EM_SETCHARFORMAT, SCF_SELECTION,
        reinterpret_cast<LPARAM>(&format));
}
inline void Highlight() {
    State& s = Data();
    if (!s.editor || s.suppress) return;
    std::wstring text = Text(s.editor);
    if (text.size() > 250000) {
        Status(L"Large script: syntax colours paused to keep editing responsive.");
        return;
    }
    CHARRANGE old = {};
    POINT scroll = {};
    SendMessageW(s.editor, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&old));
    SendMessageW(s.editor, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scroll));
    s.suppress = true;
    SendMessageW(s.editor, WM_SETREDRAW, FALSE, 0);
    Color(0, static_cast<LONG>(text.size()), RGB(223, 234, 245));
    size_t i = 0;
    int painted = 0;
    while (i < text.size() && painted < 18000) {
        size_t start = i;
        COLORREF ink = RGB(223, 234, 245);
        if (text[i] == L'-' && i+1 < text.size() && text[i+1] == L'-') {
            i += 2;
            if (i+1 < text.size() && text[i] == L'[' && text[i+1] == L'[') {
                i += 2;
                while (i+1 < text.size() && !(text[i] == L']' && text[i+1] == L']')) ++i;
                i = (std::min)(text.size(), i+2);
            } else {
                while (i < text.size() && text[i] != L'\n' && text[i] != L'\r') ++i;
            }
            ink = RGB(117, 180, 133);
        } else if (text[i] == L'"' || text[i] == L'\'') {
            wchar_t delim = text[i++];
            while (i < text.size()) {
                if (text[i] == L'\\' && i+1 < text.size()) { i+=2; continue; }
                if (text[i++] == delim) break;
            }
            ink = RGB(236, 175, 114);
        } else if (text[i] == L'[' && i+1 < text.size() && text[i+1] == L'[') {
            i += 2;
            while (i+1 < text.size() && !(text[i] == L']' && text[i+1] == L']')) ++i;
            i = (std::min)(text.size(), i+2);
            ink = RGB(236, 175, 114);
        } else if (iswdigit(text[i]) && (i == 0 || !IsWord(text[i-1]))) {
            while (i < text.size() && (IsWord(text[i]) || text[i] == L'.')) ++i;
            ink = RGB(183, 215, 131);
        } else if (iswalpha(text[i]) || text[i] == L'_') {
            while (i < text.size() && (IsWord(text[i]) || text[i] == L'.')) ++i;
            std::wstring name = text.substr(start, i-start);
            if (Key(name)) ink = RGB(125, 177, 249);
            else if (s.known.count(name)) ink = RGB(111, 213, 220);
            else continue;
        } else { ++i; continue; }
        Color(static_cast<LONG>(start), static_cast<LONG>(i), ink);
        ++painted;
    }
    SendMessageW(s.editor, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&old));
    SendMessageW(s.editor, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scroll));
    SendMessageW(s.editor, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(s.editor, NULL, FALSE);
    s.suppress = false;
}
inline std::wstring Prefix() {
    State& s = Data();
    CHARRANGE caret = {};
    SendMessageW(s.editor, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&caret));
    if (caret.cpMin != caret.cpMax) return L"";
    std::wstring text = Text(s.editor);
    size_t cursor = (std::min)(text.size(), static_cast<size_t>(caret.cpMin));
    size_t first = cursor;
    while (first && (IsWord(text[first-1]) || text[first-1] == L'.')) --first;
    return text.substr(first, cursor-first);
}
inline void Describe() {
    State& s = Data();
    int row = static_cast<int>(SendMessageW(s.list, LB_GETCURSEL, 0, 0));
    if (row == LB_ERR) return;
    int index = static_cast<int>(SendMessageW(s.list, LB_GETITEMDATA, row, 0));
    if (index < 0 || static_cast<size_t>(index) >= s.commands.size()) return;
    const Command& item = s.commands[static_cast<size_t>(index)];
    Status(item.signature + L"\r\nFrom " + item.source +
        L". Refer to the implementation for parameter details.");
}
inline void Suggest(const std::wstring& filter) {
    State& s = Data();
    if (!s.list) return;
    std::wstring query(filter);
    std::transform(query.begin(), query.end(), query.begin(), towlower);
    SendMessageW(s.list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(s.list, LB_RESETCONTENT, 0, 0);
    int count = 0;
    for (size_t i = 0; i < s.commands.size() && count < 500; ++i) {
        std::wstring name(s.commands[i].name);
        std::transform(name.begin(), name.end(), name.begin(), towlower);
        if (!query.empty() && name.find(query) == std::wstring::npos) continue;
        LRESULT row = SendMessageW(s.list, LB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(s.commands[i].signature.c_str()));
        if (row != LB_ERR && row != LB_ERRSPACE) {
            SendMessageW(s.list, LB_SETITEMDATA, row, static_cast<LPARAM>(i));
            ++count;
        }
    }
    if (count) SendMessageW(s.list, LB_SETCURSEL, 0, 0);
    SendMessageW(s.list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(s.list, NULL, FALSE);
    if (count) Describe();
}
inline void Complete() {
    State& s = Data();
    std::wstring prefix = Prefix();
    if (prefix.empty()) { Status(L"Type a function prefix before Ctrl+Space."); return; }
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), towlower);
    for (const Command& item : s.commands) {
        std::wstring candidate(item.name);
        std::transform(candidate.begin(), candidate.end(), candidate.begin(), towlower);
        if (candidate.compare(0, prefix.size(), prefix) != 0) continue;
        CHARRANGE select = {};
        SendMessageW(s.editor, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&select));
        select.cpMin = std::max<LONG>(0, select.cpMin - static_cast<LONG>(prefix.size()));
        SendMessageW(s.editor, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&select));
        SendMessageW(s.editor, EM_REPLACESEL, TRUE,
            reinterpret_cast<LPARAM>(item.name.c_str()));
        return;
    }
    Status(L"No installed GameGuru command matches this prefix.");
}
inline void InsertCommand() {
    State& s = Data();
    int row = static_cast<int>(SendMessageW(s.list, LB_GETCURSEL, 0, 0));
    if (row == LB_ERR) return;
    int index = static_cast<int>(SendMessageW(s.list, LB_GETITEMDATA, row, 0));
    if (index < 0 || static_cast<size_t>(index) >= s.commands.size()) return;
    std::wstring name = s.commands[static_cast<size_t>(index)].name + L"(";
    SetFocus(s.editor);
    SendMessageW(s.editor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(name.c_str()));
}
inline bool Pick(bool save, std::wstring& result) {
    wchar_t name[32768] = {};
    if (result.size() < 32767) wcscpy_s(name, 32768, result.c_str());
    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = Data().window;
    dialog.lpstrFilter = L"Lua (*.lua)\0*.lua\0";
    dialog.lpstrFile = name;
    dialog.nMaxFile = 32768;
    dialog.lpstrInitialDir = Data().scriptbank.c_str();
    dialog.lpstrDefExt = L"lua";
    dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST |
        (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))) return false;
    result = name;
    size_t dot = result.find_last_of(L'.');
    if (dot == std::wstring::npos || _wcsicmp(result.c_str() + dot, L".lua") != 0) {
        MessageBoxW(Data().window, L"Only .lua scripts are supported.",
            L"PW Lua Studio", MB_ICONWARNING | MB_OK);
        return false;
    }
    return true;
}
inline bool Save(bool as = false) {
    State& s = Data();
    std::wstring path(s.path);
    if ((as || path.empty()) && !Pick(true, path)) return false;
    std::wstring wide = Text(s.editor);
    std::string utf8 = Encode(wide);
    if (!wide.empty() && utf8.empty()) return false;
    std::wstring tmp = path + L".pw-writing";
    {
        std::ofstream file(tmp.c_str(), std::ios::binary | std::ios::trunc);
        if (!file || (!utf8.empty() && !file.write(utf8.data(), utf8.size()))) {
            DeleteFileW(tmp.c_str());
            MessageBoxW(s.window, L"Cannot write temporary Lua file.", L"PW Lua Studio",
                MB_OK | MB_ICONERROR);
            return false;
        }
    }
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        SYSTEMTIME now = {};
        GetLocalTime(&now);
        wchar_t stamp[48] = {};
        swprintf_s(stamp, L".pw-backup-%04u%02u%02u-%02u%02u%02u-%03u",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
            now.wSecond, now.wMilliseconds);
        if (!CopyFileW(path.c_str(), (path + stamp).c_str(), TRUE)) {
            DeleteFileW(tmp.c_str());
            MessageBoxW(s.window, L"Backup failed; existing script was not modified.",
                L"PW Lua Studio", MB_OK | MB_ICONERROR);
            return false;
        }
    }
    if (!MoveFileExW(tmp.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(tmp.c_str());
        MessageBoxW(s.window, L"Lua save failed; previous version is backed up.",
            L"PW Lua Studio", MB_OK | MB_ICONERROR);
        return false;
    }
    s.path = path; s.dirty = false; Caption();
    Status(L"Saved UTF-8 Lua. Scripts are not executed or hot-reloaded by this editor.");
    return true;
}
inline bool CanDiscard() {
    State& s = Data();
    if (!s.dirty) return true;
    int answer = MessageBoxW(s.window, L"Save modified Lua script?",
        L"PW Lua Studio", MB_YESNOCANCEL | MB_ICONQUESTION);
    return answer == IDNO || (answer == IDYES && Save());
}
inline bool LoadDocument(const std::wstring& path) {
    State& s = Data();
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) {
        MessageBoxW(s.window, (L"Unable to open Lua script:\n" + path).c_str(),
                    L"PW Lua Studio", MB_OK | MB_ICONWARNING);
        return false;
    }
    std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (bytes.size() > 1048576) {
        MessageBoxW(s.window, L"Files larger than 1 MB are not supported.",
            L"PW Lua Studio", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (bytes.compare(0, 3, "\xEF\xBB\xBF") == 0) bytes.erase(0, 3);
    std::wstring wide = Utf8(bytes);
    if (!bytes.empty() && wide.empty()) {
        MessageBoxW(s.window, L"Cannot open non-UTF-8 script without risking data loss.",
            L"PW Lua Studio", MB_OK | MB_ICONWARNING);
        return false;
    }
    s.suppress = true;
    SetWindowTextW(s.editor, wide.c_str());
    SendMessageW(s.editor, EM_EMPTYUNDOBUFFER, 0, 0);
    s.suppress = false;
    s.path = path;
    s.dirty = false;
    Caption();
    Highlight();
    SetFocus(s.editor);
    return true;
}
inline void OpenFile() {
    State& s = Data();
    if (!CanDiscard()) return;
    std::wstring path;
    if (Pick(false, path)) LoadDocument(path);
}
inline LRESULT CALLBACK EditProc(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
    if (message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000)) {
        if (w == VK_SPACE) { Complete(); return 0; }
        if (w == 'S') { Save((GetKeyState(VK_SHIFT) & 0x8000) != 0); return 0; }
        if (w == 'O') { OpenFile(); return 0; }
    }
    return CallWindowProcW(Data().originalProc, hwnd, message, w, l);
}
inline void Layout(HWND hwnd) {
    State& s = Data();
    RECT rect = {};
    GetClientRect(hwnd, &rect);
    int w = rect.right, h = rect.bottom, side = (std::max)(260, w / 3);
    int editorW = (std::max)(180, w-side-28);
    MoveWindow(GetDlgItem(hwnd, 101), 8, 7, 92, 28, TRUE);
    MoveWindow(GetDlgItem(hwnd, 102), 105, 7, 68, 28, TRUE);
    MoveWindow(GetDlgItem(hwnd, 103), 178, 7, 82, 28, TRUE);
    MoveWindow(GetDlgItem(hwnd, 104), 265, 7, 70, 28, TRUE);
    MoveWindow(s.editor, 8, 44, editorW, (std::max)(100, h-52), TRUE);
    MoveWindow(s.search, editorW+17, 44, side-5, 27, TRUE);
    MoveWindow(s.list, editorW+17, 78, side-5, (std::max)(80, h-198), TRUE);
    MoveWindow(s.detail, editorW+17, h-112, side-5, 100, TRUE);
}
inline LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
    State& s = Data();
    switch (message) {
    case WM_CREATE: {
        s.window = hwnd;
        s.font = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
        DWORD buttons = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
        CreateWindowW(L"BUTTON", L"Open...", buttons, 0,0,0,0, hwnd,(HMENU)101,NULL,NULL);
        CreateWindowW(L"BUTTON", L"Save", buttons, 0,0,0,0, hwnd,(HMENU)102,NULL,NULL);
        CreateWindowW(L"BUTTON", L"Save As", buttons, 0,0,0,0, hwnd,(HMENU)103,NULL,NULL);
        CreateWindowW(L"BUTTON", L"New", buttons, 0,0,0,0, hwnd,(HMENU)104,NULL,NULL);
        s.editor = CreateWindowExW(WS_EX_CLIENTEDGE, L"RICHEDIT50W", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
            0,0,0,0, hwnd,(HMENU)105,NULL,NULL);
        s.search = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            0,0,0,0, hwnd,(HMENU)106,NULL,NULL);
        s.list = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            0,0,0,0, hwnd,(HMENU)107,NULL,NULL);
        s.detail = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
            0,0,0,0, hwnd,(HMENU)108,NULL,NULL);
        if (!s.editor || !s.search || !s.list || !s.detail) return -1;
        for (HWND item : {s.editor, s.search, s.list, s.detail})
            SendMessageW(item, WM_SETFONT, reinterpret_cast<WPARAM>(s.font), TRUE);
        SendMessageW(s.editor, EM_SETBKGNDCOLOR, 0, RGB(24,29,39));
        SendMessageW(s.editor, EM_EXLIMITTEXT, 0, 1048576);
        s.originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(s.editor,
            GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&EditProc)));
        Discover(); Suggest(L""); Caption();
        Status(L"Ctrl+Space: complete command. Ctrl+O: open. Ctrl+S: save. Search or double-click functions on the right.");
        return 0;
    }
    case WM_SIZE: Layout(hwnd); return 0;
    case WM_TIMER:
        if (w == 1) { KillTimer(hwnd, 1); Highlight(); }
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(w), event = HIWORD(w);
        if (event == BN_CLICKED) {
            if (id == 101) { OpenFile(); return 0; }
            if (id == 102) { Save(); return 0; }
            if (id == 103) { Save(true); return 0; }
            if (id == 104) {
                if (CanDiscard()) {
                    s.suppress = true; SetWindowTextW(s.editor, L""); s.suppress = false;
                    s.path.clear(); s.dirty = false; Caption();
                }
                return 0;
            }
        }
        if (id == 105 && event == EN_CHANGE && !s.suppress) {
            s.dirty = true; Caption(); SetTimer(hwnd, 1, 280, NULL);
            std::wstring prefix = Prefix();
            if (prefix.size() > 1) Suggest(prefix);
            return 0;
        }
        if (id == 106 && event == EN_CHANGE && !s.suppress) {
            Suggest(Text(s.search)); return 0;
        }
        if (id == 107 && event == LBN_SELCHANGE) { Describe(); return 0; }
        if (id == 107 && event == LBN_DBLCLK) { InsertCommand(); return 0; }
        break;
    }
    case WM_CLOSE:
        if (CanDiscard()) DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (s.font) { DeleteObject(s.font); s.font = NULL; }
        s.editor = s.list = s.search = s.detail = s.window = NULL;
        s.originalProc = NULL; s.path.clear(); s.dirty = false;
        if (s.module) { FreeLibrary(s.module); s.module = NULL; }
        return 0; // Never PostQuitMessage: closing a tool must not terminate GameGuru.
    }
    return DefWindowProcW(hwnd, message, w, l);
}
inline void Open(HWND owner) {
    State& s = Data();
    if (s.window && IsWindow(s.window)) {
        ShowWindow(s.window, SW_RESTORE); SetForegroundWindow(s.window); return;
    }
    s.module = LoadLibraryW(L"Msftedit.dll");
    if (!s.module) {
        MessageBoxW(owner, L"Windows RichEdit is unavailable.",
            L"PW Lua Studio", MB_ICONERROR | MB_OK); return;
    }
    s.scriptbank = Root() + L"Files\\scriptbank\\";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc); wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"PWGameGuruLuaStudio";
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName,
        L"PW Lua Script Editor", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1050, 690,
        owner, NULL, wc.hInstance, NULL);
    if (!hwnd) {
        if (s.module) { FreeLibrary(s.module); s.module = NULL; }
        MessageBoxW(owner, L"Unable to open the Lua editor.",
            L"PW Lua Studio", MB_ICONERROR | MB_OK); return;
    }
    ShowWindow(hwnd, SW_SHOWNORMAL);
    SetForegroundWindow(hwnd);
    SetFocus(s.editor);
}
// Open the selected entity's actual AI-main Lua file in the existing built-in
// script editor, retaining dirty-file prompts and the current editor state.
// Only files under the installation's scriptbank are eligible.
inline bool OpenEntityScript(HWND owner, const char* entityPath) {
    Open(owner);
    State& s = Data();
    if (!s.window || !s.editor) return false;
    if (!entityPath || !*entityPath) {
        Status(L"No Lua script assigned. Use Open... to choose one from scriptbank.");
        OpenFile();
        return false;
    }
    std::string name(entityPath);
    for (char& c : name) if (c == '/') c = '\\';
    if (name.size() >= 11 && _strnicmp(name.c_str(), "scriptbank\\", 11) == 0)
        name.erase(0, 11);
    if (name.find("..") != std::string::npos || name.find(':') != std::string::npos ||
        (!name.empty() && name[0] == '\\') || name.empty() ||
        name.size() < 4 || _stricmp(name.c_str() + name.size() - 4, ".lua") != 0) {
        MessageBoxW(s.window, L"The assigned Lua path is outside scriptbank or invalid.",
                    L"PW Lua Studio", MB_OK | MB_ICONWARNING);
        return false;
    }
    // Resolve against the installed editor's scriptbank, not the process
    // working directory, which can vary when launched from Steam.
    std::wstring path = s.scriptbank + Utf8(name);
    if (_wcsicmp(path.c_str(), s.path.c_str()) == 0) {
        SetFocus(s.editor);
        return true;
    }
    if (!CanDiscard()) return false;
    return LoadDocument(path);
}

}
