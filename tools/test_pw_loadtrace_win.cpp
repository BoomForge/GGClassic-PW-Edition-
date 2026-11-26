// Isolated Windows build smoke test for PW-LoadTrace.h.
// Compile with VS2022 x64; does not need licensed GameGuru media.
#include "PW-LoadTrace.h"
#include "PW-TextureBudget.h"
#include "PW-TextureStage.h"
#include "PW-TextureMip.h"
#include "PW-TextureResidency.h"
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
    if(!TestBudget()||!TestStage()||!TestMip()||!TestResidency())return 1;
    PWLoadTraceScope testLoad("test.synthetic_load");
    Sleep(3);
    return 0;
}
