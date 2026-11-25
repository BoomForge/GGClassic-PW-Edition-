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
// Classic animation moving states: 0 idle, 1 walk, 2 back,
// 3/4 left/right strafe, 5 run, 7/8 running strafe.
struct Locomotion {
    bool moving=false,running=false,aiming=false;
    float direction=0.f,speed=1.f;
    int animation=0;
};
inline Locomotion DecideMovement(float yaw,int w,int a,int s,int d,bool sprint,
                                 bool canRun,bool aiming,bool crouching,bool allowed) {
    Locomotion out;
    out.direction=Wrap(Finite(yaw,0.f));out.aiming=aiming;
    const int x=(d==1)-(a==1),z=(w==1)-(s==1);
    if(!allowed||(!x&&!z))return out;
    out.moving=true;
    out.direction=Facing(out.direction,z>0,x<0,z<0,x>0,out.direction);
    out.running=sprint&&canRun&&!aiming&&!crouching;
    out.speed=crouching?0.5f:(aiming?0.75f:(out.running?2.f:1.f));
    if(!aiming)out.animation=out.running?5:1;
    else if(x<0)out.animation=3;
    else if(x>0)out.animation=4;
    else if(z<0)out.animation=2;
    else out.animation=1;
    return out;
}
inline int MovementAnimation(bool moving,bool running,bool aiming,float movementYaw,float facingYaw) {
    if(!moving)return 0;
    if(!aiming)return running?5:1;
    const float delta=Signed(movementYaw-facingYaw);
    if(std::fabs(delta)>135.f)return 2;
    if(delta<=-45.f)return running?7:3;
    if(delta>=45.f)return running?8:4;
    return running?5:1;
}
inline float JumpHoldFrame(float first,float last,float authored) {
    if(!std::isfinite(first)||!std::isfinite(last)||first<=0.f||last<=first)return -1.f;
    return Clamp(authored>0.f?authored:last-10.f,first,last-1.f);
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
