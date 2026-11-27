#pragma once
#include <d3d11.h>
#include <cstddef>
namespace PWMip {
constexpr UINT MaxDimension=2048, TailDimension=256;
constexpr std::size_t FrameBytes=4u*1024u*1024u;
constexpr std::size_t DecodeReserve=6u*1024u*1024u;
constexpr std::size_t MaxPending=8;
inline bool Format(DXGI_FORMAT f){
    return f==DXGI_FORMAT_BC1_UNORM||f==DXGI_FORMAT_BC1_UNORM_SRGB||
        f==DXGI_FORMAT_BC3_UNORM||f==DXGI_FORMAT_BC3_UNORM_SRGB;
}
inline UINT Tail(UINT w,UINT h,UINT levels){
    UINT mip=0;
    while(mip+1<levels&&(w>TailDimension||h>TailDimension)){
        if(w>1)w/=2;if(h>1)h/=2;++mip;
    }
    return mip;
}
inline bool Eligible(UINT w,UINT h,UINT levels,DXGI_FORMAT format){
    if(!w||!h||w>MaxDimension||h>MaxDimension||levels<2||levels>12||!Format(format))return false;
    UINT full=1,large=w>h?w:h;
    while(large>1){large/=2;++full;}
    return levels==full&&Tail(w,h,levels)>0;
}
struct Span { const void* pixels=nullptr; UINT row=0,bytes=0; };
inline bool Valid(const Span& s,UINT width,UINT height,DXGI_FORMAT fmt){
    if(!Format(fmt)||!width||!height)return false;
    const UINT block=(fmt==DXGI_FORMAT_BC1_UNORM||fmt==DXGI_FORMAT_BC1_UNORM_SRGB)?8u:16u;
    const UINT row=((width+3u)/4u)*block, rows=(height+3u)/4u;
    return s.pixels&&s.row==row&&s.bytes==row*rows&&s.bytes<=FrameBytes;
}
inline ID3D11Texture2D* Create(ID3D11Device* device,ID3D11DeviceContext* context,
    UINT w,UINT h,UINT levels,DXGI_FORMAT format,const Span* spans,UINT& first){
    first=0;
    if(!device||!context||context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||
        device->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0||!spans||!Eligible(w,h,levels,format))return nullptr;
    UINT width=w,height=h;
    for(UINT i=0;i<levels;++i){
        if(!Valid(spans[i],width,height,format))return nullptr;
        if(width>1)width/=2;if(height>1)height/=2;
    }
    UINT support=0;
    if(FAILED(device->CheckFormatSupport(format,&support))||
        !(support&D3D11_FORMAT_SUPPORT_TEXTURE2D)||!(support&D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))return nullptr;
    D3D11_TEXTURE2D_DESC desc={};
    desc.Width=w;desc.Height=h;desc.MipLevels=levels;desc.ArraySize=1;desc.Format=format;
    desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;
    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.MiscFlags=D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
    ID3D11Texture2D* texture=nullptr;
    if(FAILED(device->CreateTexture2D(&desc,nullptr,&texture))||!texture)return nullptr;
    const UINT tail=Tail(w,h,levels);
    for(UINT mip=tail;mip<levels;++mip)
        context->UpdateSubresource(texture,mip,nullptr,spans[mip].pixels,spans[mip].row,spans[mip].bytes);
    context->SetResourceMinLOD(texture,static_cast<FLOAT>(tail));
    first=tail;
    return texture;
}
// One immediate-context command sequence: make the next mip accessible,
// upload it before any subsequent draw, and never change resource/view identity.
inline bool Promote(ID3D11DeviceContext* context,ID3D11Texture2D* texture,UINT& first,const Span& s){
    if(!context||!texture||!first)return false;
    D3D11_TEXTURE2D_DESC d={};texture->GetDesc(&d);
    const UINT next=first-1;
    if(first>=d.MipLevels||!(d.MiscFlags&D3D11_RESOURCE_MISC_RESOURCE_CLAMP))return false;
    UINT w=d.Width>>next,h=d.Height>>next;
    if(!Valid(s,w?w:1,h?h:1,d.Format))return false;
    context->SetResourceMinLOD(texture,static_cast<FLOAT>(next));
    context->UpdateSubresource(texture,next,nullptr,s.pixels,s.row,s.bytes);
    first=next;return true;
}
}
