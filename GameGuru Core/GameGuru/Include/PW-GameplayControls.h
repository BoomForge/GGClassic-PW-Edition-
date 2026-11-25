#pragma once
// Physical DirectInput scan codes, separate from editor/raw Lua keys.
#include "PW-DisplayOptions.h"
#include <array>
#include <sstream>
namespace PWControls {
enum Action { Forward, Backward, Left, Right, Jump, Crouch, Use, Reload, LeanLeft, Interact, Sprint, ActionF, ActionZ, ActionJ, Count };
struct ActionInfo { int logical,setup;const wchar_t* label; };
inline const std::array<ActionInfo,Count>& Actions() {
    static const std::array<ActionInfo,Count> a={{
        {17,1,L"Move forward"},{31,2,L"Move backward"},{30,3,L"Move left"},{32,4,L"Move right"},
        {57,5,L"Jump"},{46,6,L"Crouch"},{28,7,L"Use / enter"},{19,8,L"Reload"},{16,9,L"Lean left"},
        {18,10,L"Interact / lean right"},{42,11,L"Sprint"},{33,0,L"F action"},{44,0,L"Z action"},{36,0,L"J action"}
    }};return a;
}
struct Key {int code;const wchar_t* name;};
inline const std::vector<Key>& Keys() {
    static const std::vector<Key> keys={
        {30,L"A"},{48,L"B"},{46,L"C"},{32,L"D"},{18,L"E"},{33,L"F"},{34,L"G"},{35,L"H"},
        {23,L"I"},{36,L"J"},{37,L"K"},{38,L"L"},{50,L"M"},{49,L"N"},{24,L"O"},{25,L"P"},
        {16,L"Q"},{19,L"R"},{31,L"S"},{20,L"T"},{22,L"U"},{47,L"V"},{17,L"W"},{45,L"X"},
        {21,L"Y"},{44,L"Z"},{57,L"Space"},{28,L"Enter"},{42,L"Left Shift"},{54,L"Right Shift"},
        {29,L"Left Ctrl"},{157,L"Right Ctrl"},{200,L"Up"},{208,L"Down"},{203,L"Left"},{205,L"Right"},
        {26,L"["},{27,L"]"},{39,L";"},{40,L"'"},{51,L","},{52,L"."},{53,L"/"},{43,L"\\"}
    };return keys;
}
using Bindings=std::array<int,Count>;
inline bool Allowed(int code){for(const auto& k:Keys())if(k.code==code)return true;return false;}
inline bool Valid(const Bindings& b) {
    for(size_t i=0;i<b.size();++i){
        if(!Allowed(b[i]))return false;
        for(size_t j=0;j<i;++j)if(b[i]==b[j])return false;
    }return true;
}
inline Bindings Classic() {
    Bindings b={};for(size_t i=0;i<b.size();++i)b[i]=Actions()[i].logical;return b;
}
inline std::string Path(const std::string& root){return PWLevelFlow::Root(root)+"pw_controls.ini";}
inline bool Save(const std::string& root,const Bindings& b) {
    if(!Valid(b))return false;
    std::string s="PWCONTROLS1\n";
    for(int k:b)s+=std::to_string(k)+"\n";
    return PWProfile::Atomic(Path(root),s);
}
inline bool Read(const std::string& root,Bindings& b) {
    std::string data;if(!PWProfile::ReadBytes(Path(root),data)||data.size()>512)return false;
    std::istringstream in(data);std::string version,extra;Bindings next={};
    if(!(in>>version)||version!="PWCONTROLS1")return false;
    for(auto& k:next)if(!(in>>k))return false;
    if(in>>extra||!Valid(next))return false;
    b=next;return true;
}
inline int Index(int logical){for(size_t i=0;i<Actions().size();++i)if(Actions()[i].logical==logical)return static_cast<int>(i);return -1;}
enum {First=310,Reset=330,Notice=331};
struct Dialog {Bindings selected=Classic(),defaults=Classic();std::string root;};
inline void Populate(HWND w,const Bindings& b) {
    for(size_t i=0;i<b.size();++i){
        for(size_t k=0;k<Keys().size();++k)if(Keys()[k].code==b[i]){
            SendDlgItemMessageW(w,First+static_cast<int>(i),CB_SETCURSEL,k,0);break;
        }
    }
}
inline INT_PTR CALLBACK Proc(HWND w,UINT msg,WPARAM wp,LPARAM lp) {
    Dialog* d=reinterpret_cast<Dialog*>(GetWindowLongPtrW(w,DWLP_USER));
    if(msg==WM_INITDIALOG){
        d=reinterpret_cast<Dialog*>(lp);SetWindowLongPtrW(w,DWLP_USER,reinterpret_cast<LONG_PTR>(d));
        for(int i=0;i<Count;++i)for(const auto& k:Keys())SendDlgItemMessageW(w,First+i,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(k.name));
        Populate(w,d->selected);return TRUE;
    }
    if(!d)return FALSE;
    if(msg==WM_CLOSE){EndDialog(w,IDCANCEL);return TRUE;}
    if(msg==WM_COMMAND){
        if(LOWORD(wp)==Reset){Populate(w,d->defaults);SetDlgItemTextW(w,Notice,L"Defaults selected. Save to apply.");return TRUE;}
        if(LOWORD(wp)==IDCANCEL){EndDialog(w,IDCANCEL);return TRUE;}
        if(LOWORD(wp)==IDOK){
            Bindings next={};
            for(int i=0;i<Count;++i){
                const LRESULT k=SendDlgItemMessageW(w,First+i,CB_GETCURSEL,0,0);
                if(k<0||static_cast<size_t>(k)>=Keys().size())return TRUE;
                next[static_cast<size_t>(i)]=Keys()[static_cast<size_t>(k)].code;
            }
            if(!Valid(next)){SetDlgItemTextW(w,Notice,L"Conflict: each action needs a different key.");return TRUE;}
            if(!Save(d->root,next)){SetDlgItemTextW(w,Notice,L"Could not save controls. No changes applied.");return TRUE;}
            d->selected=next;EndDialog(w,IDOK);return TRUE;
        }
    }return FALSE;
}
inline bool Configure(HWND owner,Dialog& d) {
    PWDisplay::Template t(L"Gameplay controls",static_cast<WORD>(Count*2+4),245);
    for(int i=0;i<Count;++i){
        const short y=static_cast<short>(8+i*14);
        t.Item(static_cast<WORD>(400+i),0x82,SS_LEFT,9,y+2,105,12,Actions()[static_cast<size_t>(i)].label);
        t.Item(static_cast<WORD>(First+i),0x85,WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,115,y,125,90,L"");
    }
    t.Item(Notice,0x82,SS_LEFT,9,207,232,15,L"Mouse fire/aim and controller bindings unchanged.");
    t.Item(Reset,0x80,WS_TABSTOP|BS_PUSHBUTTON,9,226,80,15,L"Reset defaults");
    t.Item(IDOK,0x80,WS_TABSTOP|BS_DEFPUSHBUTTON,111,226,61,15,L"Save");
    t.Item(IDCANCEL,0x80,WS_TABSTOP|BS_PUSHBUTTON,179,226,61,15,L"Cancel");
    return DialogBoxIndirectParamW(GetModuleHandleW(nullptr),t.Data(),owner,Proc,reinterpret_cast<LPARAM>(&d))==IDOK;
}
}
