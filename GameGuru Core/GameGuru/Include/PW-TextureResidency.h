#pragma once
#include <cstdint>
#include <cmath>
namespace PWResidency {
constexpr unsigned PromoteDelay=250,DemoteDelay=1000,UnseenDelay=2000;
inline bool Pressure(bool previous,bool valid,std::uint64_t usage,std::uint64_t budget){
    if(!valid||!budget)return false;
    const unsigned threshold=previous?80u:90u;
    return usage>=(budget/100)*threshold+((budget%100)*threshold)/100;
}
inline unsigned Desired(unsigned current,unsigned maxMip,float distance,bool seen,bool pressure){
    if(!seen||!std::isfinite(distance)||distance<600.0f)return 0;
    const float scale=pressure?0.65f:1.0f;
    unsigned target=0;const float threshold[]={800.0f,1800.0f,3500.0f};
    for(unsigned i=0;i<3&&i<maxMip;++i){
        if(distance>=threshold[i]*scale*(current>i?0.85f:1.15f))target=i+1;
        else break;
    }
    return target;
}
struct State {unsigned current=0,requested=0;std::uint64_t since=0;};
inline bool Step(State& s,unsigned target,std::uint64_t now){
    if(target==s.current){s.requested=target;s.since=now;return false;}
    if(target!=s.requested||now<s.since){s.requested=target;s.since=now;return false;}
    if(now-s.since<(target<s.current?PromoteDelay:DemoteDelay))return false;
    s.current=target<s.current?target:s.current+1;s.since=now;return true;
}
}
