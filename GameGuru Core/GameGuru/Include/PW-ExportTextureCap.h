#pragma once
// Export-only cap: retain the original DDS mip payload, pixel format and channels.
// No decompression, resampling, source-media mutation, or encryption bypass.
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>
namespace PWExportTextureCap {
using U32=std::uint32_t; using U64=std::uint64_t;
inline U32 Read(const unsigned char* h, unsigned i) {
    return U32(h[i]) | (U32(h[i+1])<<8) | (U32(h[i+2])<<16) | (U32(h[i+3])<<24);
}
inline void Write(unsigned char* h,unsigned i,U32 v) {
    for(unsigned j=0;j<4;++j) h[i+j]=static_cast<unsigned char>(v>>(8*j));
}
inline U32 Four(char a,char b,char c,char d) {return U32(a)|(U32(b)<<8)|(U32(c)<<16)|(U32(d)<<24);}
struct Plan { unsigned char header[128]; U64 offset=0, outputBytes=0; };
inline U64 Size(U32 w,U32 h,U32 block) {
    return block?U64((w+3)/4)*((h+3)/4)*block:U64(w)*h*4;
}
inline bool MakePlan(const unsigned char* h,U64 bytes,U32 cap,Plan& out) {
    if(!h||bytes<128||cap<4||Read(h,0)!=Four('D','D','S',' ')||Read(h,4)!=124||Read(h,76)!=32) return false;
    U32 flags=Read(h,8), height=Read(h,12), width=Read(h,16), depth=Read(h,24), mips=Read(h,28);
    U32 pf=Read(h,80), four=Read(h,84), bits=Read(h,88), caps=Read(h,108);
    if(!width||!height||width>16384||height>16384||depth>1||Read(h,112)||Read(h,116)||Read(h,120)||(flags&0x800000)||mips<2||(std::max)(width,height)<=cap) return false;
    U32 maxmips=1; for(U32 x=(std::max)(width,height);x>1;x>>=1) ++maxmips;
    if(mips>maxmips) return false;
    U32 block=0;
    if(pf&4) {
        if(four==Four('D','X','T','1')) block=8;
        else if(four==Four('D','X','T','3')||four==Four('D','X','T','5')) block=16;
        else if(four!=63) return false; // legacy signed 32-bit normal format
    } else if(!(pf&0x40)||bits!=32) return false;
    U64 total=128,skip=0; U32 dropped=0,w=width,hh=height;
    for(U32 i=0;i<mips;++i) {
        U64 size=Size(w,hh,block); total+=size;
        if((std::max)(w,hh)>cap) {skip+=size;++dropped;}
        w=(std::max)(1u,w>>1);hh=(std::max)(1u,hh>>1);
    }
    if(total!=bytes||!dropped||dropped>=mips) return false;
    w=(std::max)(1u,width>>dropped);hh=(std::max)(1u,height>>dropped);
    std::memcpy(out.header,h,128);
    Write(out.header,12,hh);Write(out.header,16,w);Write(out.header,28,mips-dropped);
    if(flags&0x80000) Write(out.header,20,static_cast<U32>(Size(w,hh,block)));
    else if(flags&8) Write(out.header,20,block?((w+3)/4)*block:w*4);
    if(mips-dropped==1){Write(out.header,8,flags&~0x20000u);Write(out.header,108,caps&~(0x400000u|8u));}
    out.offset=128+skip;out.outputBytes=bytes-skip;
    return true;
}
inline bool IsEntityDDS(std::string name) {
    for(char& c:name) {if(c=='/')c='\\';if(c>='A'&&c<='Z')c=char(c-'A'+'a');}
    return name.size()>15&&name.compare(0,11,"entitybank\\")==0&&name.compare(name.size()-4,4,".dds")==0;
}
#ifdef _WIN32
// The caller supplies only the freshly copied standalone destination.
inline U64 CapCopiedFile(const char* relative,const char* destination,U32 cap) {
    if(!relative||!destination||!IsEntityDDS(relative))return 0;
    HANDLE in=CreateFileA(destination,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(in==INVALID_HANDLE_VALUE)return 0;
    unsigned char header[128]; DWORD got=0; LARGE_INTEGER size={};Plan plan;
    if(!GetFileSizeEx(in,&size)||size.QuadPart<128||!ReadFile(in,header,128,&got,NULL)||got!=128||!MakePlan(header,static_cast<U64>(size.QuadPart),cap,plan)){CloseHandle(in);return 0;}
    std::string temp=std::string(destination)+".pw-mip.tmp";
    HANDLE dst=CreateFileA(temp.c_str(),GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(dst==INVALID_HANDLE_VALUE){CloseHandle(in);return 0;}
    DWORD wrote=0;bool ok=WriteFile(dst,plan.header,128,&wrote,NULL)&&wrote==128;
    LARGE_INTEGER seek={};seek.QuadPart=plan.offset;ok=ok&&SetFilePointerEx(in,seek,NULL,FILE_BEGIN);
    unsigned char buffer[65536];U64 left=plan.outputBytes-128;
    while(ok&&left){DWORD n=static_cast<DWORD>((std::min)(left,U64(sizeof(buffer))));ok=ReadFile(in,buffer,n,&got,NULL)&&got==n&&WriteFile(dst,buffer,n,&wrote,NULL)&&wrote==n;if(ok)left-=n;}
    ok=ok&&FlushFileBuffers(dst);CloseHandle(dst);CloseHandle(in);
    if(ok&&MoveFileExA(temp.c_str(),destination,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return static_cast<U64>(size.QuadPart)-plan.outputBytes;
    DeleteFileA(temp.c_str());return 0;
}
#endif
}
