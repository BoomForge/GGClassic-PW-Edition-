#pragma once
// One logical profile per name. Win32 bounded, checked, recoverable writes.
// No GameGuru globals or Lua execution; the engine validates gameplay data.
#include "PW-LevelFlow.h"
#include <cstdint>
#include <string>
#include <vector>
namespace PWProfile {
constexpr size_t MaxPayload=160*1024;
inline bool Name(const std::string& name) {
    if(name.empty() || name.size()>32) return false;
    for(char c:name) if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-')) return false;
    const std::string lower=PWLevelFlow::Lower(name);
    if(lower=="con"||lower=="prn"||lower=="aux"||lower=="nul") return false;
    if(lower.size()==4 && (lower.substr(0,3)=="com"||lower.substr(0,3)=="lpt") && lower[3]>='0'&&lower[3]<='9') return false;
    return true;
}
struct Snapshot { std::string level,payload; bool recovered=false; };
inline uint32_t Checksum(const std::string& data) {
    uint32_t h=2166136261u;
    for(unsigned char c:data){h^=c;h*=16777619u;}
    return h;
}
inline bool Encode(const Snapshot& s,std::string& out) {
    if(!PWLevelFlow::SafeRequestedName(s.level)||s.level.find('\n')!=std::string::npos||
        s.payload.empty()||s.payload.size()>MaxPayload||s.payload.find('\0')!=std::string::npos) return false;
    const std::string body=s.level+"\n"+s.payload;
    out="PWPROFILE1\n"+std::to_string(Checksum(body))+"\n"+body;
    return true;
}
inline bool Decode(const std::string& bytes,Snapshot& out) {
    if(bytes.size()>MaxPayload+300 || bytes.compare(0,11,"PWPROFILE1\n")!=0) return false;
    const size_t end=bytes.find('\n',11);
    if(end==std::string::npos || end<=11 || end-11>10) return false;
    uint64_t sum=0;
    for(size_t i=11;i<end;++i){char c=bytes[i];if(c<'0'||c>'9')return false;sum=sum*10+uint64_t(c-'0');}
    const std::string body=bytes.substr(end+1);
    if(sum!=Checksum(body))return false;
    const size_t line=body.find('\n');
    if(line==std::string::npos)return false;
    Snapshot next;
    next.level=body.substr(0,line);next.payload=body.substr(line+1);
    if(!PWLevelFlow::SafeRequestedName(next.level)||next.payload.empty()||next.payload.size()>MaxPayload||next.payload.find('\0')!=std::string::npos)return false;
    out=next;return true;
}
inline bool Regular(const std::string& path) {
    DWORD a=GetFileAttributesA(path.c_str());
    return a!=INVALID_FILE_ATTRIBUTES && !(a&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT));
}
inline bool Directory(const std::string& path,bool create) {
    DWORD a=GetFileAttributesA(path.c_str());
    if(a==INVALID_FILE_ATTRIBUTES && create) {
        if(!CreateDirectoryA(path.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS)return false;
        a=GetFileAttributesA(path.c_str());
    }
    return a!=INVALID_FILE_ATTRIBUTES && (a&FILE_ATTRIBUTE_DIRECTORY) && !(a&FILE_ATTRIBUTE_REPARSE_POINT);
}
inline std::string Root(const std::string& files,bool create) {
    const std::string save=PWLevelFlow::Root(files)+"savegames";
    const std::string root=save+"\\pw_profiles";
    if(root.size()+50>=MAX_PATH||!Directory(save,create)||!Directory(root,create))return {};
    return root;
}
inline std::string Path(const std::string& root,const std::string& name) {
    return root.empty()||!Name(name)?std::string():PWLevelFlow::Root(root)+PWLevelFlow::Lower(name)+".pwprofile";
}
inline bool ReadBytes(const std::string& path,std::string& out) {
    if(!Regular(path))return false;
    HANDLE h=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER size={};DWORD n=0;
    bool ok=GetFileSizeEx(h,&size)!=0 && size.QuadPart>0 && size.QuadPart<=static_cast<LONGLONG>(MaxPayload+300);
    std::string bytes;
    if(ok){bytes.resize(static_cast<size_t>(size.QuadPart));ok=ReadFile(h,&bytes[0],static_cast<DWORD>(bytes.size()),&n,nullptr)!=0&&n==bytes.size();}
    CloseHandle(h);
    if(ok)out.swap(bytes);return ok;
}
inline bool Atomic(const std::string& path,const std::string& bytes) {
    if(path.empty()||bytes.empty())return false;
    DWORD a=GetFileAttributesA(path.c_str());
    if(a!=INVALID_FILE_ATTRIBUTES && !Regular(path))return false;
    // Unique CREATE_NEW temporary file in the same directory/volume.
    std::string tmp;
    HANDLE h=INVALID_HANDLE_VALUE;
    for(unsigned i=0;i<16;++i){
        tmp=path+".tmp."+std::to_string(GetCurrentProcessId())+"."+std::to_string(GetTickCount())+"."+std::to_string(i);
        if(tmp.size()>=MAX_PATH)return false;
        h=CreateFileA(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(h!=INVALID_HANDLE_VALUE)break;
        if(GetLastError()!=ERROR_FILE_EXISTS && GetLastError()!=ERROR_ALREADY_EXISTS)return false;
    }
    if(h==INVALID_HANDLE_VALUE)return false;
    DWORD n=0;
    bool ok=WriteFile(h,bytes.data(),static_cast<DWORD>(bytes.size()),&n,nullptr)!=0 && n==bytes.size() && FlushFileBuffers(h)!=0;
    if(!CloseHandle(h))ok=false;
    if(ok)ok=MoveFileExA(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileA(tmp.c_str());
    return ok;
}
inline bool Save(const std::string& root,const std::string& name,const Snapshot& snapshot) {
    const std::string path=Path(root,name);
    std::string bytes,old;Snapshot verified;
    if(path.empty()||!Encode(snapshot,bytes))return false;
    // Only a previously valid primary may replace the recovery copy.
    // Never overwrite a good recovery file with a corrupt primary.
    if(ReadBytes(path,old)&&Decode(old,verified)&&!Atomic(path+".bak",old))return false;
    return Atomic(path,bytes);
}
inline bool Load(const std::string& root,const std::string& name,Snapshot& out) {
    const std::string path=Path(root,name);std::string bytes;Snapshot decoded;
    if(path.empty())return false;
    if(ReadBytes(path,bytes)&&Decode(bytes,decoded)){out=decoded;return true;}
    if(ReadBytes(path+".bak",bytes)&&Decode(bytes,decoded)){decoded.recovered=true;out=decoded;return true;}
    return false;
}
inline std::vector<std::string> List(const std::string& root) {
    std::vector<std::string> result;if(root.empty())return result;
    WIN32_FIND_DATAA f={};HANDLE h=FindFirstFileA((PWLevelFlow::Root(root)+"*.pwprofile").c_str(),&f);
    if(h==INVALID_HANDLE_VALUE)return result;
    do{
        std::string n=f.cFileName;
        if(!(f.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))&&n.size()>10&&PWLevelFlow::Lower(n.substr(n.size()-10))==".pwprofile"){
            n.resize(n.size()-10);if(Name(n))result.push_back(n);
        }
    }while(result.size()<256&&FindNextFileA(h,&f));
    FindClose(h);std::sort(result.begin(),result.end());return result;
}
inline bool CurrentLevel(const std::string& mapbank,std::string project, PWLevelFlow::Destination& out) {
    std::replace(project.begin(),project.end(),'/','\\');
    const std::string root=PWLevelFlow::Root(mapbank);
    const std::string low=PWLevelFlow::Lower(project);
    if(low.compare(0,8,"mapbank\\")==0)project.erase(0,8);
    else if(!root.empty()&&low.compare(0,root.size(),PWLevelFlow::Lower(root))==0)project.erase(0,root.size());
    return PWLevelFlow::Resolve(mapbank,project,out);
}
}
