#pragma once
#include <cmath>
namespace PWThirdPerson {
struct Vec {float x,y,z;};
inline Vec Add(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec Sub(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec Scale(Vec a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float Dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float Length(Vec a){return std::sqrt(Dot(a,a));}
inline float Clamp(float x,float a,float b){return x<a?a:(x>b?b:x);}
inline float Finite(float x,float fallback){return std::isfinite(x)?x:fallback;}
inline float Radians(float x){return x*0.017453292519943295f;}
inline float Wrap(float a){a=std::fmod(a,360.f);return a<0?a+360.f:a;}
inline float Signed(float a){a=Wrap(a);return a>180.f?a-360.f:a;}
inline float ApproachAngle(float from,float target,float dt) {
    return Wrap(from+Signed(target-from)*Clamp(1.f-std::exp(-12.f*dt),0.f,1.f));
}
inline float Facing(float yaw,int w,int a,int s,int d,float previous){
    const int x=(d!=0)-(a!=0),z=(w!=0)-(s!=0);
    if(!x&&!z)return previous;
    return Wrap(yaw+std::atan2(static_cast<float>(x),static_cast<float>(z))*57.29577951308232f);
}
struct Orbit {Vec offset,right,up;};
inline Orbit Build(float yaw,float pitch,float distance,float shoulder,float height,bool locked){
    yaw=Radians(Finite(yaw,0));pitch=Radians(Clamp(Signed(Finite(pitch,0)),-65.f,80.f));
    distance=Clamp(Finite(distance,200),35.f,1000.f);
    shoulder=Clamp(Finite(shoulder,0),-100.f,100.f);
    height=Clamp(Finite(height,100),-100.f,500.f);
    const float sy=std::sin(yaw),cy=std::cos(yaw),sp=std::sin(pitch),cp=std::cos(pitch);
    const Vec right={cy,0,-sy};
    const Vec up={sy*sp,cp,cy*sp};
    const Vec behind={-sy*cp,sp,-cy*cp};
    Vec offset=Add(Scale(behind,distance),Scale(right,shoulder));
    if(locked)offset=Add({-sy*distance,height,-cy*distance},Scale(right,shoulder));
    return {offset,right,up};
}
// Keep the camera along its tested boom; never ease through an obstacle.
inline float ContactFraction(Vec start,Vec end,Vec hit,float padding){
    const Vec delta=Sub(end,start);const float l=Length(delta);
    if(l<=0.0001f)return 0.f;
    return Clamp((Dot(Sub(hit,start),delta)/l-padding)/l,0.f,1.f);
}
inline float Boom(float previous,float allowed,float dt,float speed){
    allowed=Clamp(allowed,0.f,1.f);previous=Clamp(previous,0.f,1.f);
    if(allowed<=previous)return allowed;
    return previous+(allowed-previous)*Clamp(1.f-std::exp(-Clamp(speed,1.f,25.f)*Clamp(dt,0.f,0.1f)),0.f,1.f);
}
}
