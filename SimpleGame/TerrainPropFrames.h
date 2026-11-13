#pragma once
namespace TerrainPropFrames
{
constexpr int TextureWidth = 1254, TextureHeight = 1254;
struct Frame { int x,y,width,height; float footX,footY,bodyHeight; };
constexpr Frame Frames[4] = {
    {0,0,627,627,310.50000000f,607.00000000f,587.00000000f},
    {627,0,627,627,312.50000000f,544.00000000f,422.00000000f},
    {0,627,627,627,310.00000000f,600.00000000f,580.00000000f},
    {627,627,627,627,307.00000000f,595.00000000f,587.00000000f},
};
}
