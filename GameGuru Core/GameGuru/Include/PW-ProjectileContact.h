#pragma once
#include <cmath>
// Pure finite segment/sphere intersection used by direct projectile collision.
// Return the first contact parameter in [0,1], not an infinite ray hit.
namespace PWProjectile {
inline bool FirstContact(float ax,float ay,float az,float bx,float by,float bz,
                         float cx,float cy,float cz,float radius,float& time) {
    const double dx=double(bx)-ax,dy=double(by)-ay,dz=double(bz)-az;
    const double x=double(ax)-cx,y=double(ay)-cy,z=double(az)-cz;
    const double a=dx*dx+dy*dy+dz*dz;
    const double b=x*dx+y*dy+z*dz;
    const double c=x*x+y*y+z*z-double(radius)*radius;
    if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c)||!(radius>0)) return false;
    if(c<=0){time=0;return true;}
    if(a<=1e-12 || b>=0) return false;
    const double d=b*b-a*c;
    if(d<0) return false;
    const double hit=(-b-std::sqrt(d))/a;
    if(hit<0 || hit>1) return false;
    time=float(hit);
    return true;
}
}
