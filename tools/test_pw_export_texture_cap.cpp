#include "../GameGuru Core/GameGuru/Include/PW-ExportTextureCap.h"
#include <cassert>
#include <iostream>
using namespace PWExportTextureCap;
int main(){
    unsigned char h[128]={};Write(h,0,Four('D','D','S',' '));Write(h,4,124);Write(h,8,0xA1007);Write(h,12,4096);Write(h,16,4096);Write(h,28,13);Write(h,76,32);Write(h,80,4);Write(h,84,Four('D','X','T','5'));Write(h,108,0x401008);
    U64 size=128;for(unsigned w=4096;;w>>=1){size+=Size(w,w,16);if(w==1)break;}
    Plan p;assert(MakePlan(h,size,2048,p));assert(Read(p.header,12)==2048&&Read(p.header,16)==2048&&Read(p.header,28)==12);assert(p.offset==128+Size(4096,4096,16));assert(p.outputBytes==size-Size(4096,4096,16));assert(Read(p.header,84)==Read(h,84));assert(!MakePlan(h,size-1,2048,p));assert(!MakePlan(h,size,4096,p));Write(h,112,0x200);assert(!MakePlan(h,size,2048,p));Write(h,112,0);Write(h,84,Four('D','X','1','0'));assert(!MakePlan(h,size,2048,p));
    assert(IsEntityDDS("Entitybank/test.DDS"));assert(!IsEntityDDS("gamecore/a.dds"));assert(!IsEntityDDS("entitybank/a.png"));
    std::cout<<"PASS export mip cap: format, dimensions, payload boundaries and exclusions\n";
}
