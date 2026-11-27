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
// Lower scores win. Use physical player reach and facing, never orbit-camera distance.
inline float InteractionScore(Vec origin,Vec target,float yaw,float range) {
    if(!InReach(origin,target,range)||!std::isfinite(yaw))return -1.f;
    const Vec d=Sub(target,origin);
    const float flat=d.x*d.x+d.z*d.z;
    const float angle=flat<1.f?0.f:std::fabs(Signed(std::atan2(d.x,d.z)*57.29577951308232f-yaw));
    // Close items directly beneath the player remain accessible.
    if(angle>80.f&&flat>400.f)return -1.f;
    return Length(d)/range+angle/180.f;
}
inline bool BetterInteraction(float score,int entity,float best,int chosen) {
    return std::isfinite(score)&&score>=0.f&&entity>0&&
        (chosen==0||score<best||(score==best&&entity<chosen));
}
struct InteractionPress {
    bool held=true; // require release at level entry
    int Step(bool down,bool allowed,int target) {
        const bool rising=down&&!held;
        held=down;
        return rising&&allowed&&target>0?target:0;
    }
};
inline bool Direction(Vec delta,Vec& result) {
    const float length=Length(delta);
    if(!std::isfinite(length)||length<0.001f)return false;
    result=Scale(delta,1.f/length);return true;
}
inline Vec ForwardVector(float yaw,float pitch) {
    const float y=Radians(yaw),p=Radians(pitch),c=std::cos(p);
    return {std::sin(y)*c,-std::sin(p),std::cos(y)*c};
}
struct Aim {Vec muzzle={},target={},direction={};float yaw=0,pitch=0,range=0;};
inline bool ResolveAim(Vec muzzle,Vec target,float range,Aim& aim) {
    Vec direction={};
    if(!std::isfinite(range)||range<=0.f||!Direction(Sub(target,muzzle),direction))return false;
    aim.muzzle=muzzle;aim.target=target;aim.direction=direction;aim.range=range;
    aim.yaw=Wrap(std::atan2(direction.x,direction.z)*57.29577951308232f);
    aim.pitch=std::atan2(-direction.y,std::sqrt(direction.x*direction.x+direction.z*direction.z))*57.29577951308232f;
    return true;
}
bool BuildPlayerAim(float range,Aim& aim);

// Zero means holster, positive requests must be owned and preloaded.
inline bool EquipAllowed(int requested,int maxWeapon,bool owned,bool hudReady) {
    return requested==0||(requested>0&&requested<=maxWeapon&&owned&&hudReady);
}
// Validate the whole inventory before applying a saved state. A missing
// held weapon or HUD is not a reason to grant another weapon.
inline bool ValidInventoryIds(const int (&slots)[10],int maxWeapon) {
    for(int id:slots)if(id<0||id>maxWeapon)return false;
    return true;
}
inline int RestoredWeapon(int requested,const int (&slots)[10],int maxWeapon,bool hudReady) {
    if(requested<=0||requested>maxWeapon||!hudReady)return 0;
    for(int id:slots)if(id==requested)return requested;
    return 0;
}
struct EquipStage {int entity=0,weapon=0,oldObject=0,oldLimb=0,newObject=0,newLimb=0;bool unchanged=false;};
bool NativeLifecycleEnabled();
bool CaptureBodyCheckpoint(float yaw);
bool PrepareEquip(int weapon,EquipStage& stage);
void FinishEquip(const EquipStage& stage,bool commit);
}
