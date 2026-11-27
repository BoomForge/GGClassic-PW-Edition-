#pragma once
// M5B isolated D3D11.2 backend. No live GameGuru image registry admission yet.
// One stable tiled resource, one private pool, one independently reclaimable
// finest mip. Coarse and packed mips are permanently backed.
#include "PW-TextureMip.h"
#include <d3d11_2.h>
#include <wrl/client.h>
namespace PWTiled {
constexpr UINT TileBytes=65536, MaxTiles=128;
struct Layout {
    UINT total=0, finest=0;
    bool Valid() const {return finest>0&&finest<total&&total<=MaxTiles;}
    UINT Coarse() const {return Valid()?total-finest:0;}
    UINT Bytes(bool full) const {return Valid()?(full?total:Coarse())*TileBytes:0;}
};
inline bool Supported(ID3D11Device* device){
    D3D11_FEATURE_DATA_D3D11_OPTIONS1 options={};
    return device&&SUCCEEDED(device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1,&options,sizeof(options)))&&
        options.TiledResourcesTier>=D3D11_TILED_RESOURCES_TIER_2;
}
class Texture {
    using TexturePtr=Microsoft::WRL::ComPtr<ID3D11Texture2D>;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> ctx_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> pool_;
    TexturePtr tex_;
    Layout layout_;
    UINT allocated_=0;
    bool finestMapped_=false,ready_=false;
    bool Healthy() const {return device_&&SUCCEEDED(device_->GetDeviceRemovedReason());}
    HRESULT MapFinest(bool mapped){
        D3D11_TILED_RESOURCE_COORDINATE start={};
        D3D11_TILE_REGION_SIZE size={};size.NumTiles=layout_.finest;
        const UINT flag=mapped?0u:static_cast<UINT>(D3D11_TILE_RANGE_NULL);
        const UINT offset=layout_.Coarse(),count=layout_.finest;
        return ctx_->UpdateTileMappings(tex_.Get(),1,&start,&size,mapped?pool_.Get():nullptr,1,&flag,&offset,&count,0);
    }
    bool Resize(UINT bytes){
        if(allocated_==bytes)return true;
        if(FAILED(ctx_->ResizeTilePool(pool_.Get(),bytes)))return false;
        allocated_=bytes;return Healthy();
    }
public:
    Texture()=default;
    Texture(const Texture&)=delete;
    Texture& operator=(const Texture&)=delete;
    ID3D11Texture2D* Resource() const {return ready_?tex_.Get():nullptr;}
    UINT PoolBytes() const {return allocated_;}
    bool Full() const {return finestMapped_;}
    UINT CoarseBytes() const {return layout_.Bytes(false);}
    bool Initialize(ID3D11Device* device,ID3D11DeviceContext* context,UINT w,UINT h,UINT levels,DXGI_FORMAT format,const PWMip::Span* spans){
        if(device_||!Supported(device)||!context||context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||
            !spans||!PWMip::Eligible(w,h,levels,format))return false;
        Microsoft::WRL::ComPtr<ID3D11Device> owner;context->GetDevice(owner.GetAddressOf());
        if(owner.Get()!=device)return false;
        for(UINT i=0;i<levels;++i){
            const UINT mw=w>>i,mh=h>>i;
            if(!PWMip::Valid(spans[i],mw?mw:1,mh?mh:1,format))return false;
        }
        Microsoft::WRL::ComPtr<ID3D11Device2> d2;
        if(FAILED(device->QueryInterface(IID_PPV_ARGS(d2.GetAddressOf())))||
            FAILED(context->QueryInterface(IID_PPV_ARGS(ctx_.GetAddressOf()))))return false;
        device_=device;
        D3D11_TEXTURE2D_DESC desc={};
        desc.Width=w;desc.Height=h;desc.MipLevels=levels;desc.ArraySize=1;desc.Format=format;
        desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags=D3D11_RESOURCE_MISC_TILED|D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
        if(FAILED(device->CreateTexture2D(&desc,nullptr,tex_.GetAddressOf())))return false;
        D3D11_PACKED_MIP_DESC packed={};D3D11_SUBRESOURCE_TILING top={};
        UINT count=1,total=0;
        d2->GetResourceTiling(tex_.Get(),&total,&packed,nullptr,&count,0,&top);
        if(count!=1||!packed.NumStandardMips||top.StartTileIndexInOverallResource!=0||
            !top.WidthInTiles||!top.HeightInTiles||top.DepthInTiles!=1)return false;
        const UINT64 tiles=static_cast<UINT64>(top.WidthInTiles)*top.HeightInTiles;
        if(tiles>MaxTiles)return false;
        layout_.total=total;layout_.finest=static_cast<UINT>(tiles);
        if(!layout_.Valid())return false;
        D3D11_BUFFER_DESC poolDesc={};
        poolDesc.ByteWidth=layout_.Bytes(true);poolDesc.Usage=D3D11_USAGE_DEFAULT;poolDesc.MiscFlags=D3D11_RESOURCE_MISC_TILE_POOL;
        if(FAILED(device->CreateBuffer(&poolDesc,nullptr,pool_.GetAddressOf())))return false;
        allocated_=poolDesc.ByteWidth;
        // Whole logical resource in native order, physical ranges reversed:
        // highest-resolution mip at the end of the pool, all other tiles first.
        // Includes the packed mip tail without guessing its internal layout.
        const UINT offsets[]={layout_.Coarse(),0},counts[]={layout_.finest,layout_.Coarse()};
        if(FAILED(ctx_->UpdateTileMappings(tex_.Get(),1,nullptr,nullptr,pool_.Get(),2,nullptr,offsets,counts,0)))return false;
        for(UINT i=0;i<levels;++i)ctx_->UpdateSubresource(tex_.Get(),i,nullptr,spans[i].pixels,spans[i].row,spans[i].bytes);
        ctx_->SetResourceMinLOD(tex_.Get(),0.0f);
        finestMapped_=true;ready_=Healthy();return ready_;
    }
    bool Demote(){
        if(!ready_||!Healthy()||!tex_||!pool_||!layout_.Valid())return false;
        if(finestMapped_){
            ctx_->SetResourceMinLOD(tex_.Get(),1.0f);
            if(FAILED(MapFinest(false))){ctx_->SetResourceMinLOD(tex_.Get(),0.0f);return false;}
            finestMapped_=false;
        }
        // Only count a real accepted shrink; failure retains coarse detail,
        // allows retry, and never claims the original allocation was freed.
        return Resize(layout_.Bytes(false));
    }
    bool Promote(const PWMip::Span& original){
        if(!ready_||!Healthy()||!tex_||!pool_||!layout_.Valid())return false;
        if(finestMapped_)return true;
        D3D11_TEXTURE2D_DESC desc={};tex_->GetDesc(&desc);
        if(!PWMip::Valid(original,desc.Width,desc.Height,desc.Format))return false;
        if(!Resize(layout_.Bytes(true)))return false;
        if(FAILED(MapFinest(true))){Resize(layout_.Bytes(false));return false;}
        // D3D11 clamps also restrict update/copy access. Lower it only
        // after successful remapping, then upload before any following draw.
        ctx_->SetResourceMinLOD(tex_.Get(),0.0f);
        ctx_->UpdateSubresource(tex_.Get(),0,nullptr,original.pixels,original.row,original.bytes);
        if(!Healthy()){ready_=false;return false;}
        finestMapped_=true;return true;
    }
};
}
