// Isolated Windows build smoke test for PW-LoadTrace.h.
// Compile with VS2022 x64; does not need licensed GameGuru media.
#include "PW-LoadTrace.h"
#include "PW-TextureBudget.h"
#include "PW-TextureStage.h"
static bool TestStage(){
    using namespace PWTextureStage;
    if(!Fits(128,BatchLimit)||Fits(127,BatchLimit)||Fits(FileLimit+1,BatchLimit)||Fits(256,128))return false;
    char folder[MAX_PATH]={},path[MAX_PATH]={};
    const DWORD n=GetTempPathA(MAX_PATH,folder);
    if(!n||n>=MAX_PATH||!GetTempFileNameA(folder,"pws",0,path))return false;
    unsigned char contents[256]={'D','D','S',' '};
    HANDLE file=CreateFileA(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    DWORD written=0;
    bool ok=file!=INVALID_HANDLE_VALUE&&WriteFile(file,contents,sizeof(contents),&written,nullptr)&&written==sizeof(contents);
    if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
    std::atomic<bool> cancelled{false};std::vector<std::uint8_t> data;BY_HANDLE_FILE_INFORMATION stamp={},now={};
    ok=ok&&Read(path,BatchLimit,cancelled,data,stamp)&&data.size()==sizeof(contents)&&data[0]=='D';
    {File current(path);ok=ok&&current.Info(now)&&SameFile(stamp,now);}
    cancelled.store(true);ok=ok&&!Read(path,BatchLimit,cancelled,data,stamp)&&data.empty();
    cancelled.store(false);ok=ok&&!Read(path,128,cancelled,data,stamp)&&data.empty();
    ++now.nFileSizeLow;ok=ok&&!SameFile(stamp,now);
    DeleteFileA(path);return ok;
}
static bool TestBudget(){
    using namespace PWTextureBudget;
    Bytes parsed=0;
    if(!ParseMiB("512",parsed)||parsed!=512*MiB||ParseMiB("-1",parsed)||ParseMiB("999999999999",parsed)||ParseMiB("1x",parsed))return false;
    if(!ParseMiB("0",parsed)||parsed)return false;
    Input in;
    if(Evaluate(in).targetKnown)return false;
    in.dedicated=1000*MiB;in.registeredGpu=800*MiB;
    auto out=Evaluate(in);
    if(out.target!=700*MiB||out.logicalExcess!=100*MiB||out.adapterPressure)return false;
    in.localValid=true;in.localBudget=500*MiB;in.localUsage=480*MiB;in.configured=900*MiB;
    out=Evaluate(in);
    if(out.target!=350*MiB||out.logicalExcess!=450*MiB||!out.adapterPressure||out.adapterExcess!=30*MiB)return false;
    in.configured=200*MiB;if(Evaluate(in).target!=200*MiB)return false;
    Candidate a;a.id=1;a.generation=7;a.reclaimable=20*MiB;a.priority=Priority::World;
    a.pinned=false;a.exclusiveOwnership=true;a.reloadable=true;
    Candidate b=a;b.id=2;b.priority=Priority::Background;b.lastUsed=5;
    Candidate c=b;c.id=3;c.lastUsed=2;
    Candidate pinned=c;pinned.id=4;pinned.pinned=true;
    Candidate borrowed=c;borrowed.id=5;borrowed.exclusiveOwnership=false;
    Candidate duplicate=c;duplicate.id=6;
    Candidate essential=c;essential.id=7;essential.priority=Priority::Essential;
    Candidate stale=c;stale.id=8;stale.generation=0;
    Candidate missing=c;missing.id=9;missing.reloadable=false;
    auto plan=PlanEviction({a,b,c,pinned,borrowed,duplicate,duplicate,essential,stale,missing},30*MiB);
    if(plan.selected.size()!=2||plan.selected[0].id!=3||plan.selected[1].id!=2||plan.remaining||plan.planned!=40*MiB)return false;
    plan=PlanEviction({pinned,borrowed,essential,stale,missing},100*MiB);
    if(!plan.selected.empty()||plan.remaining!=100*MiB)return false;
    return PlanEviction({a},0).selected.empty()&&Add((std::numeric_limits<Bytes>::max)(),1)==(std::numeric_limits<Bytes>::max)();
}
int main()
{
    if(!TestBudget()||!TestStage())return 1;
    PWLoadTraceScope testLoad("test.synthetic_load");
    Sleep(3);
    return 0;
}
