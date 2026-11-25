#pragma once
// Native, pre-game standalone display configuration. No engine/UI globals.
// Borderless desktop fullscreen avoids exclusive display/OS mode changes.
#include "PW-PlayerProfile.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
namespace PWDisplay {
struct Choice { int width=1280,height=720; bool fullscreen=true; };
struct Size { int width,height; };
inline bool Same(const Choice& a,const Choice& b) {
    return a.width==b.width&&a.height==b.height&&a.fullscreen==b.fullscreen;
}
inline bool Valid(const Choice& c,const std::vector<Size>& modes,int desktopW,int desktopH) {
    if(c.fullscreen)return c.width==desktopW&&c.height==desktopH;
    for(const auto& mode:modes)if(mode.width==c.width&&mode.height==c.height)return true;
    return false;
}
inline std::vector<Size> Modes(int maxW,int maxH) {
    std::vector<Size> sizes;
    auto add=[&](int w,int h) {
        if(w<640||h<480||w>maxW||h>maxH)return;
        for(const auto& s:sizes)if(s.width==w&&s.height==h)return;
        sizes.push_back({w,h});
    };
    DEVMODEA mode={};mode.dmSize=sizeof(mode);
    for(DWORD i=0;EnumDisplaySettingsA(nullptr,i,&mode);++i) {
        if(mode.dmBitsPerPel>=24)add(static_cast<int>(mode.dmPelsWidth),static_cast<int>(mode.dmPelsHeight));
        mode.dmSize=sizeof(mode);
    }
    // Windowed backbuffers need not be physical monitor modes.
    const Size common[]={{640,480},{800,600},{1024,576},{1024,768},{1280,720},{1366,768},{1600,900},{1920,1080},{2560,1440}};
    for(const auto& s:common)add(s.width,s.height);
    std::sort(sizes.begin(),sizes.end(),[](const Size& a,const Size& b){return a.width==b.width?a.height<b.height:a.width<b.width;});
    return sizes;
}
inline Choice Desktop(int w,int h) {Choice c;c.width=w;c.height=h;c.fullscreen=true;return c;}
inline Choice Safe(const std::vector<Size>& modes,int w,int h) {
    Choice c=Desktop(w,h);
    for(const auto& s:modes) {
        if(s.width<=1280&&s.height<=720){c.width=s.width;c.height=s.height;c.fullscreen=false;}
    }
    if(c.fullscreen&&!modes.empty()){c.width=modes.front().width;c.height=modes.front().height;c.fullscreen=false;}
    return c;
}
inline std::string SettingsPath(const std::string& root) {return PWLevelFlow::Root(root)+"pw_display.ini";}
inline std::string PendingPath(const std::string& root) {return PWLevelFlow::Root(root)+"pw_display.pending";}
inline bool Read(const std::string& root,Choice& c) {
    std::string data;
    if(!PWProfile::ReadBytes(SettingsPath(root),data)||data.size()>128)return false;
    int w=0,h=0,f=-1;char extra=0;
    if(sscanf_s(data.c_str(),"PWDISPLAY1 %d %d %d %c",&w,&h,&f,&extra,1u)!=3||w<640||h<480||w>16384||h>16384||(f!=0&&f!=1))return false;
    c.width=w;c.height=h;c.fullscreen=f==1;return true;
}
inline bool Save(const std::string& root,const Choice& c) {
    return PWProfile::Atomic(SettingsPath(root),"PWDISPLAY1 "+std::to_string(c.width)+" "+std::to_string(c.height)+" "+(c.fullscreen?"1\n":"0\n"));
}
inline bool MarkPending(const std::string& root) {return PWProfile::Atomic(PendingPath(root),"pending\n");}
inline bool HasPending(const std::string& root) {return GetFileAttributesA(PendingPath(root).c_str())!=INVALID_FILE_ATTRIBUTES;}
inline void ClearPending(const std::string& root) {if(PWProfile::Regular(PendingPath(root)))DeleteFileA(PendingPath(root).c_str());}
// Standard Win32 dialog template, generated in memory to avoid new .rc resources.
class Template {
    std::vector<WORD> words;
    void Align(){if(words.size()%2)words.push_back(0);}
    void Text(const wchar_t* s){while(*s)words.push_back(static_cast<WORD>(*s++));words.push_back(0);}
    void Dword(DWORD n){words.push_back(LOWORD(n));words.push_back(HIWORD(n));}
public:
    Template(const wchar_t* caption,WORD count,short height) {
        Dword(WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_SETFONT|DS_CENTER);
        Dword(0);words.push_back(count);
        words.push_back(0);words.push_back(0);words.push_back(250);words.push_back(static_cast<WORD>(height));
        words.push_back(0);words.push_back(0);Text(caption);words.push_back(9);Text(L"Segoe UI");
    }
    void Item(WORD id,WORD klass,DWORD style,short x,short y,short w,short h,const wchar_t* text) {
        Align();Dword(style|WS_CHILD|WS_VISIBLE);Dword(0);
        words.push_back(static_cast<WORD>(x));words.push_back(static_cast<WORD>(y));
        words.push_back(static_cast<WORD>(w));words.push_back(static_cast<WORD>(h));words.push_back(id);
        words.push_back(0xffff);words.push_back(klass);Text(text);words.push_back(0);
    }
    const DLGTEMPLATE* Data()const {return reinterpret_cast<const DLGTEMPLATE*>(words.data());}
};
enum {ModeId=101,ResolutionId=102,InfoId=103};
struct Dialog {Choice selection;std::vector<Size> modes;int desktopW=0,desktopH=0;bool keep=false;int seconds=15;};
inline void UpdateMode(HWND window,Dialog& d) {
    bool full=SendDlgItemMessageW(window,ModeId,CB_GETCURSEL,0,0)==1;
    EnableWindow(GetDlgItem(window,ResolutionId),full?FALSE:TRUE);
    SetDlgItemTextW(window,InfoId,full?L"Borderless fullscreen uses your desktop resolution.":L"Choose a resolution that fits your desktop.");
    (void)d;
}
inline INT_PTR CALLBACK ConfigureProc(HWND window,UINT msg,WPARAM wp,LPARAM lp) {
    Dialog* d=reinterpret_cast<Dialog*>(GetWindowLongPtrW(window,DWLP_USER));
    if(msg==WM_INITDIALOG) {
        d=reinterpret_cast<Dialog*>(lp);SetWindowLongPtrW(window,DWLP_USER,reinterpret_cast<LONG_PTR>(d));
        SendDlgItemMessageW(window,ModeId,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Windowed"));
        SendDlgItemMessageW(window,ModeId,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Fullscreen (borderless)"));
        SendDlgItemMessageW(window,ModeId,CB_SETCURSEL,d->selection.fullscreen?1:0,0);
        int selected=0;
        for(size_t i=0;i<d->modes.size();++i) {
            const auto& s=d->modes[i];const std::wstring label=std::to_wstring(s.width)+L" x "+std::to_wstring(s.height);
            SendDlgItemMessageW(window,ResolutionId,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));
            if(s.width==d->selection.width&&s.height==d->selection.height)selected=static_cast<int>(i);
        }
        SendDlgItemMessageW(window,ResolutionId,CB_SETCURSEL,selected,0);
        if(d->modes.empty())SendDlgItemMessageW(window,ModeId,CB_SETCURSEL,1,0);
        UpdateMode(window,*d);return TRUE;
    }
    if(!d)return FALSE;
    if(msg==WM_CLOSE){EndDialog(window,IDCANCEL);return TRUE;}
    if(msg==WM_COMMAND){
        if(LOWORD(wp)==ModeId&&HIWORD(wp)==CBN_SELCHANGE){
            if(d->modes.empty())SendDlgItemMessageW(window,ModeId,CB_SETCURSEL,1,0);
            UpdateMode(window,*d);return TRUE;
        }
        if(LOWORD(wp)==IDCANCEL){EndDialog(window,IDCANCEL);return TRUE;}
        if(LOWORD(wp)==IDOK){
            bool full=SendDlgItemMessageW(window,ModeId,CB_GETCURSEL,0,0)==1;
            if(full)d->selection=Desktop(d->desktopW,d->desktopH);
            else {
                const LRESULT i=SendDlgItemMessageW(window,ResolutionId,CB_GETCURSEL,0,0);
                if(i<0||static_cast<size_t>(i)>=d->modes.size())return TRUE;
                d->selection.fullscreen=false;d->selection.width=d->modes[static_cast<size_t>(i)].width;d->selection.height=d->modes[static_cast<size_t>(i)].height;
            }
            EndDialog(window,IDOK);return TRUE;
        }
    }
    return FALSE;
}
inline bool Configure(Dialog& d) {
    Template t(L"Display options",7,117);
    t.Item(201,0x82,SS_LEFT,9,8,230,12,L"Choose your display settings");
    t.Item(202,0x82,SS_LEFT,9,26,58,12,L"Display mode");
    t.Item(ModeId,0x85,WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,72,24,168,64,L"");
    t.Item(ResolutionId,0x85,WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,72,46,168,78,L"");
    t.Item(InfoId,0x82,SS_LEFT,9,70,232,19,L"");
    t.Item(IDOK,0x80,WS_TABSTOP|BS_DEFPUSHBUTTON,111,94,61,15,L"Start game");
    t.Item(IDCANCEL,0x80,WS_TABSTOP|BS_PUSHBUTTON,179,94,61,15,L"Cancel");
    return DialogBoxIndirectParamW(GetModuleHandleW(nullptr),t.Data(),nullptr,ConfigureProc,reinterpret_cast<LPARAM>(&d))==IDOK;
}
inline void Countdown(HWND window,Dialog& d) {
    const std::wstring text=L"Keep these settings? Reverting in "+std::to_wstring(d.seconds)+L" seconds.";
    SetDlgItemTextW(window,InfoId,text.c_str());
}
inline INT_PTR CALLBACK ConfirmProc(HWND window,UINT msg,WPARAM wp,LPARAM lp) {
    Dialog* d=reinterpret_cast<Dialog*>(GetWindowLongPtrW(window,DWLP_USER));
    if(msg==WM_INITDIALOG){
        d=reinterpret_cast<Dialog*>(lp);SetWindowLongPtrW(window,DWLP_USER,reinterpret_cast<LONG_PTR>(d));
        Countdown(window,*d);if(!SetTimer(window,1,1000,nullptr))EndDialog(window,IDCANCEL);return TRUE;
    }
    if(!d)return FALSE;
    if(msg==WM_TIMER&&wp==1){if(--d->seconds<=0){KillTimer(window,1);EndDialog(window,IDCANCEL);}else Countdown(window,*d);return TRUE;}
    if(msg==WM_CLOSE||(msg==WM_COMMAND&&(LOWORD(wp)==IDOK||LOWORD(wp)==IDCANCEL))){
        KillTimer(window,1);EndDialog(window,msg==WM_COMMAND&&LOWORD(wp)==IDOK?IDOK:IDCANCEL);return TRUE;
    }
    return FALSE;
}
inline bool Confirm(HWND owner,Dialog& d) {
    Template t(L"Confirm display settings",3,57);
    t.Item(InfoId,0x82,SS_LEFT,9,9,232,16,L"");
    t.Item(IDOK,0x80,WS_TABSTOP|BS_PUSHBUTTON,111,33,61,15,L"Keep");
    t.Item(IDCANCEL,0x80,WS_TABSTOP|BS_DEFPUSHBUTTON,179,33,61,15,L"Revert");
    d.seconds=15;
    return DialogBoxIndirectParamW(GetModuleHandleW(nullptr),t.Data(),owner,ConfirmProc,reinterpret_cast<LPARAM>(&d))==IDOK;
}
}
