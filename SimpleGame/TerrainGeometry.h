#pragma once
#include "Monster.h"
#include <algorithm>
#include <cmath>
#include <string>

struct TerrainStrip
{
    std::string id;
    int kind = 0; // 0 mountain pass, 1 stone bridge, 2 cliff walk
    Vec2 start = {}, end = {};
    float halfWidth = 3, height = 5;
};
namespace TerrainGeometry
{
constexpr float HeightPixels = 20;
inline float Length(const TerrainStrip &strip)
{
    return std::hypot(strip.end.x-strip.start.x, strip.end.z-strip.start.z);
}
inline Vec2 Local(const TerrainStrip &strip, Vec2 p)
{
    const float length=Length(strip), dx=(strip.end.x-strip.start.x)/length, dz=(strip.end.z-strip.start.z)/length;
    return {(p.x-strip.start.x)*dx+(p.z-strip.start.z)*dz,
        -(p.x-strip.start.x)*dz+(p.z-strip.start.z)*dx};
}
inline float Smooth(float value)
{
    value=std::max(0.0f,std::min(1.0f,value));
    return value*value*(3-2*value);
}
inline float Height(const TerrainStrip &strip, Vec2 p)
{
    const auto local=Local(strip,p);
    const float length=Length(strip);
    if (local.x<0 || local.x>length) return 0;
    const float edge=std::abs(local.z);
    if (edge>strip.halfWidth+(strip.kind==0 ? 6 : 0)) return 0;
    const float ramp=std::min(Smooth(local.x/(length*.22f)),Smooth((length-local.x)/(length*.22f)));
    const float side=strip.kind==0 ? 1-Smooth((edge-strip.halfWidth)/6) : 1;
    return strip.height*ramp*side;
}
inline bool Blocked(const TerrainStrip &strip, Vec2 p)
{
    if (strip.kind==0) return false;
    const auto local=Local(strip,p);
    const float length=Length(strip), edge=std::abs(local.z);
    return local.x>length*.04f && local.x<length*.96f && edge>strip.halfWidth && edge<strip.halfWidth+8;
}
}
