// Isolated Windows build smoke test for PW-LoadTrace.h.
// Compile with VS2022 x64; does not need licensed GameGuru media.
#include "PW-LoadTrace.h"
#include "PW-TextureBudget.h"
#include "PW-TextureStage.h"
#include "PW-TextureMip.h"
#include "PW-TextureResidency.h"
#include "PW-TextureTiled.h"
#include "PW-TextureBenchmark.h"
#include <dxgi1_4.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
#include <cstdio>
static bool TestResidency(){
    using namespace PWResidency;
    if(Pressure(false,true,89,100)||!Pressure(false,true,90,100)||!Pressure(true,true,85,100)||Pressure(true,true,79,100)||Pressure(true,false,100,100))return false;
    if(Desired(0,3,500,true,true)||Desired(2,3,500,true,true)||Desired(0,3,10000,false,true)||Desired(0,1,10000,true,false)!=1)return false;
    if(Desired(0,3,850,true,false)!=0||Desired(1,3,850,true,false)!=1||Desired(0,3,1000,true,false)!=1||Desired(0,3,700,true,true)!=1)return false;
    State s;
    if(Step(s,3,100)||Step(s,3,1099)||!Step(s,3,1100)||s.current!=1||Step(s,3,1500)||!Step(s,3,2100)||s.current!=2)return false;
    if(Step(s,0,2200)||Step(s,0,2449)||!Step(s,0,2450)||s.current)return false;
    return true;
}
#include <wrl/client.h>
#pragma comment(lib,"d3d11.lib")
static bool TestMip(){
    using Microsoft::WRL::ComPtr;
    using namespace PWMip;
    if(Tail(1024,512,11)!=2||Tail(128,128,8)!=0||
        !Eligible(512,512,10,DXGI_FORMAT_BC1_UNORM)||Eligible(4096,4096,13,DXGI_FORMAT_BC3_UNORM)||
        Eligible(512,512,9,DXGI_FORMAT_BC1_UNORM))return false;
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> ctx;
    const D3D_FEATURE_LEVEL level=D3D_FEATURE_LEVEL_11_0;
    if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,&level,1,D3D11_SDK_VERSION,
        device.GetAddressOf(),nullptr,ctx.GetAddressOf())))return false;
    std::vector<std::uint8_t> pixels[10];Span spans[10]={};
    UINT dim=512;
    for(UINT mip=0;mip<10;++mip){
        const UINT blocks=(dim+3)/4;
        pixels[mip].resize(static_cast<std::size_t>(blocks)*blocks*8,static_cast<std::uint8_t>(mip+1));
        spans[mip].pixels=pixels[mip].data();spans[mip].row=blocks*8;
        spans[mip].bytes=static_cast<UINT>(pixels[mip].size());
        if(dim>1)dim/=2;
    }
    UINT first=0;ComPtr<ID3D11Texture2D> tex;
    tex.Attach(Create(device.Get(),ctx.Get(),512,512,10,DXGI_FORMAT_BC1_UNORM,spans,first));
    if(!tex||first!=1||ctx->GetResourceMinLOD(tex.Get())!=1.0f)return false;
    ComPtr<ID3D11ShaderResourceView> view;
    if(FAILED(device->CreateShaderResourceView(tex.Get(),nullptr,view.GetAddressOf())))return false;
    if(!Promote(ctx.Get(),tex.Get(),first,spans[0])||first||ctx->GetResourceMinLOD(tex.Get())!=0.0f)return false;
    ComPtr<ID3D11Resource> fromView;view->GetResource(fromView.GetAddressOf());
    if(fromView.Get()!=tex.Get())return false;
    D3D11_TEXTURE2D_DESC desc={};tex->GetDesc(&desc);
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> readback;
    if(FAILED(device->CreateTexture2D(&desc,nullptr,readback.GetAddressOf())))return false;
    ctx->SetResourceMinLOD(tex.Get(),1.0f);
    if(ctx->GetResourceMinLOD(tex.Get())!=1.0f)return false;
    ctx->SetResourceMinLOD(tex.Get(),0.0f);
    if(ctx->GetResourceMinLOD(tex.Get())!=0.0f)return false;
    ctx->CopyResource(readback.Get(),tex.Get());
    D3D11_MAPPED_SUBRESOURCE map={};
    if(FAILED(ctx->Map(readback.Get(),0,D3D11_MAP_READ,0,&map)))return false;
    bool ok=map.RowPitch>=spans[0].row;
    for(UINT row=0;ok&&row<128;++row){
        const auto* data=static_cast<const std::uint8_t*>(map.pData)+static_cast<std::size_t>(row)*map.RowPitch;
        for(UINT i=0;i<spans[0].row;++i)if(data[i]!=1){ok=false;break;}
    }
    ctx->Unmap(readback.Get(),0);
    return ok;
}

static bool TestBenchmark(){
    using namespace PWTextureBenchmark;
    const char* approved="C:\\Games\\GameGuru\\Files\\entitybank\\user\\PWStreamingApproved\\props\\crate_D.dds";
    if(!ApprovedDiffusePath(approved,true)||ApprovedDiffusePath(approved,false)||
        !ApprovedDiffusePath("D:/GG/Files/entitybank/user/PWStreamingApproved/rock_d.dds",true)||
        ApprovedDiffusePath("C:\\GG\\Files\\entitybank\\user\\other\\rock_d.dds",true)||
        ApprovedDiffusePath("C:\\GG\\Files\\entitybank\\user\\PWStreamingApproved\\..\\rock_d.dds",true)||
        ApprovedDiffusePath("C:\\GG\\Files\\entitybank\\user\\PWStreamingApproved\\grass_d.dds",true)||
        ApprovedDiffusePath("C:\\GG\\Files\\entitybank\\user\\PWStreamingApproved\\crate_n.dds",true))return false;
    Window window;window.Add(5);window.Add(5.1);window.Add(16);window.Add(300);window.Add(-1);
    return window.count==4&&window.bins[0]==1&&window.bins[1]==1&&window.bins[4]==1&&
        window.bins[11]==1&&window.max==300&&std::abs(window.sum-326.1)<0.001;
}
// Flush submitted GPU work using a bounded event query. Do not report
// post-shrink usage before queued copies and mappings have completed.
static bool BenchmarkFence(ID3D11Device* device,ID3D11DeviceContext* ctx){
    Microsoft::WRL::ComPtr<ID3D11Query> query;
    D3D11_QUERY_DESC desc={D3D11_QUERY_EVENT,0};
    if(FAILED(device->CreateQuery(&desc,query.GetAddressOf())))return false;
    ctx->End(query.Get());ctx->Flush();
    const ULONGLONG begin=GetTickCount64();
    for(;;){
        BOOL complete=FALSE;
        const HRESULT hr=ctx->GetData(query.Get(),&complete,sizeof(complete),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_OK&&complete)return true;
        if(FAILED(hr)||GetTickCount64()-begin>=5000)return false;
        Sleep(1);
    }
}
static bool BenchmarkHardwareSample(ID3D11Device* device,ID3D11DeviceContext* ctx,
    const char* phase,unsigned cycle,unsigned pool,ULONGLONG operationMs){
    using Microsoft::WRL::ComPtr;
    if(!BenchmarkFence(device,ctx))return false;
    ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;ComPtr<IDXGIAdapter3> adapter3;
    DXGI_QUERY_VIDEO_MEMORY_INFO info={};
    bool valid=SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(dxgi.GetAddressOf())))&&
        SUCCEEDED(dxgi->GetAdapter(adapter.GetAddressOf()))&&
        SUCCEEDED(adapter.As(&adapter3))&&
        SUCCEEDED(adapter3->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&info))&&info.Budget;
    PROCESS_MEMORY_COUNTERS_EX pm={};pm.cb=sizeof(pm);
    const BOOL ram=GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pm),sizeof(pm));
    printf("M6_SAMPLE,%s,%u,%u,%d,%llu,%llu,%d,%llu,%llu,%llu\n",phase,cycle,pool,valid?1:0,
        (unsigned long long)info.CurrentUsage,(unsigned long long)info.Budget,ram?1:0,
        (unsigned long long)pm.WorkingSetSize,(unsigned long long)pm.PrivateUsage,(unsigned long long)operationMs);
    return true;
}
static bool TestTiled(){
    using Microsoft::WRL::ComPtr;
    // Opt-in only; no reclaim for pinned, stale, unregistered, invisible
    // or nearby use, even with high DXGI pressure.
    if(PWTiled::Target(false,false,true,true,true,5000,true,0)||
       PWTiled::Target(true,true,true,true,true,5000,true,0)||
       PWTiled::Target(true,false,false,true,true,5000,true,0)||
       PWTiled::Target(true,false,true,false,true,5000,true,0)||
       PWTiled::Target(true,false,true,true,false,5000,true,0)||
       PWTiled::Target(true,false,true,true,true,500,true,1)||
       PWTiled::Target(true,false,true,true,true,5000,false,0)!=1)return false;
    PWTiled::Layout layout;layout.total=20;layout.finest=16;
    if(!layout.Valid()||layout.Bytes(true)!=20*65536u||layout.Bytes(false)!=4*65536u)return false;
    layout.finest=20;if(layout.Valid()||layout.Bytes(false))return false;
    layout.total=129;layout.finest=1;if(layout.Valid())return false;
    // Real hardware only. Hosted CI often has no tiled-capable adapter.
    // GGCPW_REQUIRE_TILED_TEST=1 turns unsupported into an explicit failure.
    char required[8]={};
    const bool mustRun=GetEnvironmentVariableA("GGCPW_REQUIRE_TILED_TEST",required,sizeof(required))==1&&required[0]=='1';
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> ctx;
    const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0};
    HRESULT hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,levels,2,D3D11_SDK_VERSION,
        device.GetAddressOf(),nullptr,ctx.GetAddressOf());
    if(FAILED(hr)||!PWTiled::Supported(device.Get())){
        printf("M5B hardware tile mapping SKIPPED: tier 2 adapter unavailable\n");
        return !mustRun;
    }
    printf("M6_SAMPLE_HEADER,phase,cycle,pool_bytes,dxgi_valid,dxgi_usage,dxgi_budget,ram_valid,working_set,private_bytes,operation_ms\n");
    if(!BenchmarkHardwareSample(device.Get(),ctx.Get(),"before",0,0,0))return false;
    constexpr UINT size=1024,mips=11;
    std::vector<std::uint8_t> pixels[mips];PWMip::Span spans[mips]={};
    UINT dim=size;
    for(UINT mip=0;mip<mips;++mip){
        const UINT blocks=(dim+3)/4;
        pixels[mip].resize(static_cast<std::size_t>(blocks)*blocks*16,static_cast<std::uint8_t>(mip+17));
        spans[mip].pixels=pixels[mip].data();spans[mip].row=blocks*16;
        spans[mip].bytes=static_cast<UINT>(pixels[mip].size());
        if(dim>1)dim/=2;
    }
    PWTiled::Texture tex;
    if(!tex.Initialize(device.Get(),ctx.Get(),size,size,mips,DXGI_FORMAT_BC3_UNORM,spans))return false;
    ComPtr<ID3D11ShaderResourceView> view;
    if(FAILED(device->CreateShaderResourceView(tex.Resource(),nullptr,view.GetAddressOf())))return false;
    D3D11_TEXTURE2D_DESC desc={};tex.Resource()->GetDesc(&desc);
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> readback;
    if(FAILED(device->CreateTexture2D(&desc,nullptr,readback.GetAddressOf())))return false;
    const auto check=[&](UINT mip){
        ctx->CopySubresourceRegion(readback.Get(),mip,0,0,0,tex.Resource(),mip,nullptr);
        D3D11_MAPPED_SUBRESOURCE map={};
        if(FAILED(ctx->Map(readback.Get(),mip,D3D11_MAP_READ,0,&map)))return false;
        bool ok=map.RowPitch>=spans[mip].row;
        const UINT rows=spans[mip].bytes/spans[mip].row;
        for(UINT row=0;ok&&row<rows;++row){
            const auto* data=static_cast<const std::uint8_t*>(map.pData)+static_cast<std::size_t>(row)*map.RowPitch;
            for(UINT col=0;col<spans[mip].row;++col)if(data[col]!=mip+17){ok=false;break;}
        }
        ctx->Unmap(readback.Get(),mip);return ok;
    };
    const UINT full=tex.PoolBytes(),coarse=tex.CoarseBytes();
    if(!coarse||coarse>=full||!check(0)||!check(mips-1))return false;
    if(!BenchmarkHardwareSample(device.Get(),ctx.Get(),"full",0,full,0))return false;
    for(int cycle=0;cycle<3;++cycle){
        const ULONGLONG demoteStart=GetTickCount64();
        if(!tex.Demote()||tex.Full()||tex.PoolBytes()!=coarse||ctx->GetResourceMinLOD(tex.Resource())!=1.0f||
            !check(1)||!check(mips-1))return false;
        if(!BenchmarkHardwareSample(device.Get(),ctx.Get(),"coarse",cycle+1,coarse,GetTickCount64()-demoteStart))return false;
        PWMip::Span invalid={};
        if(tex.Promote(invalid)||tex.Full()||tex.PoolBytes()!=coarse)return false;
        const ULONGLONG promoteStart=GetTickCount64();
        if(!tex.Promote(spans[0])||!tex.Full()||tex.PoolBytes()!=full||
            ctx->GetResourceMinLOD(tex.Resource())!=0.0f||!check(0)||!check(mips-1))return false;
        if(!BenchmarkHardwareSample(device.Get(),ctx.Get(),"restored",cycle+1,full,GetTickCount64()-promoteStart))return false;
        ComPtr<ID3D11Resource> identity;view->GetResource(identity.GetAddressOf());
        if(identity.Get()!=tex.Resource())return false;
    }
    printf("M5B hardware tile mapping PASSED: pool %u -> %u -> %u bytes (not DXGI usage)\n",full,coarse,full);
    return SUCCEEDED(device->GetDeviceRemovedReason());
}

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
    if(!TestBudget()||!TestStage()||!TestMip()||!TestResidency()||!TestBenchmark()||!TestTiled())return 1;
    PWLoadTraceScope testLoad("test.synthetic_load");
    Sleep(3);
    return 0;
}
