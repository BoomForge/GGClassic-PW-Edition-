#pragma once
#include "PW-ThirdPerson.h"
#include <cstdint>
// Read-only scene queries. Implemented in G-Entity.cpp; no camera or
// visibility mutation. Geometry tests shared by melee and interaction.
namespace PWThirdPerson {
inline bool InReach(Vec origin,Vec target,float range) {
    if(!std::isfinite(range)||range<=0.f)return false;
    const Vec d=Sub(target,origin);const float dist2=Dot(d,d);
    return std::isfinite(dist2)&&dist2<=range*range;
}
inline bool InMeleeArc(Vec origin,Vec target,float yaw,float range,float halfAngle,float height) {
    const Vec d=Sub(target,origin);
    if(!std::isfinite(d.x)||!std::isfinite(d.y)||!std::isfinite(d.z)||
        !std::isfinite(yaw)||!std::isfinite(range)||!std::isfinite(halfAngle)||!std::isfinite(height)||
        range<=0.f||height<0.f||halfAngle<0.f||std::fabs(d.y)>height||
        d.x*d.x+d.z*d.z>range*range)return false;
    const float flat=d.x*d.x+d.z*d.z;
    if(flat<0.0001f)return true;
    const float angle=std::atan2(d.x,d.z)*57.29577951308232f;
    return std::fabs(Signed(angle-yaw))<=Clamp(halfAngle,0.f,180.f);
}
inline int StrikeDamage(int first,int last,unsigned roll) {
    // Clamp invalid negative settings and tolerate inverted or fixed ranges.
    const int low=first<0?0:first,high=last<0?0:last;
    const int a=low<high?low:high,b=low<high?high:low;
    const uint64_t span=static_cast<uint64_t>(b)-static_cast<uint64_t>(a)+1u;
    return static_cast<int>(static_cast<uint64_t>(a)+static_cast<uint64_t>(roll)%span);
}
bool PlayerOrigin(Vec& origin,int& ignoredObject);
bool EntityTarget(int e,Vec& target,int& object);
bool LineClear(Vec from,Vec to,int ignoredObject,int targetObject);
bool CanInteract(int e,float range);
}
