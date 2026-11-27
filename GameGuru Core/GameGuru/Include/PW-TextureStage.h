#pragma once
// Bounded, cancellable CPU-side file staging. No graphics calls.
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <vector>
namespace PWTextureStage {
constexpr std::size_t MiB=1024u*1024u;
constexpr std::size_t BatchLimit=64u*MiB, FileLimit=16u*MiB, HintLimit=128u;
inline bool Fits(std::uint64_t bytes,std::size_t remaining){
    return bytes>=128&&bytes<=FileLimit&&bytes<=remaining;
}
struct File {
    HANDLE handle=INVALID_HANDLE_VALUE;
    explicit File(const char* path):handle(CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)){}
    ~File(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}
    File(const File&)=delete;
    File& operator=(const File&)=delete;
    bool Info(BY_HANDLE_FILE_INFORMATION& info)const{
        return handle!=INVALID_HANDLE_VALUE&&GetFileType(handle)==FILE_TYPE_DISK&&
            GetFileInformationByHandle(handle,&info)&&!(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY);
    }
};
inline std::uint64_t Size(const BY_HANDLE_FILE_INFORMATION& i){
    return (static_cast<std::uint64_t>(i.nFileSizeHigh)<<32)|i.nFileSizeLow;
}
inline bool SameFile(const BY_HANDLE_FILE_INFORMATION& a,const BY_HANDLE_FILE_INFORMATION& b){
    return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&
        a.nFileIndexLow==b.nFileIndexLow&&Size(a)==Size(b)&&
        a.ftLastWriteTime.dwHighDateTime==b.ftLastWriteTime.dwHighDateTime&&
        a.ftLastWriteTime.dwLowDateTime==b.ftLastWriteTime.dwLowDateTime;
}
inline bool Read(const char* path,std::size_t remaining,const std::atomic<bool>& cancel,
    std::vector<std::uint8_t>& output,BY_HANDLE_FILE_INFORMATION& stamp){
    output.clear();
    if(cancel.load())return false;
    File file(path);
    BY_HANDLE_FILE_INFORMATION before={},after={};
    if(!file.Info(before)||!Fits(Size(before),remaining))return false;
    std::vector<std::uint8_t> data(static_cast<std::size_t>(Size(before)));
    std::size_t offset=0;
    while(offset<data.size()){
        if(cancel.load())return false;
        const std::size_t left=data.size()-offset;
        const DWORD chunk=static_cast<DWORD>(left>256u*1024u?256u*1024u:left);
        DWORD got=0;
        if(!ReadFile(file.handle,data.data()+offset,chunk,&got,nullptr)||got!=chunk)return false;
        offset+=got;
    }
    if(cancel.load()||!file.Info(after)||!SameFile(before,after))return false;
    // DDS magic; full format and mip metadata validation belongs to the decoder.
    if(data[0]!='D'||data[1]!='D'||data[2]!='S'||data[3]!=' ')return false;
    stamp=after;output.swap(data);return true;
}
}
