#pragma once
#include "GameContent.h"
#include <algorithm>

namespace WorldBiomes
{
inline bool IsDesertRegion(const std::string &id) { return id == "desert_wastes" || id == "sand_outpost"; }
inline float DesertWeight(Vec2 p)
{
    float weight = 0;
    for (const auto &region : GameContent::Get().story.regions)
    {
        if (!IsDesertRegion(region.id)) continue;
        const float edge = std::min(std::min(p.x-region.minimumX, region.maximumX-p.x),
            std::min(p.z-region.minimumZ, region.maximumZ-p.z));
        const float blend = std::max(0.0f, std::min(1.0f, (edge+12)/24));
        weight = std::max(weight, blend*blend*(3-2*blend));
    }
    return weight;
}
inline bool IsDesert(Vec2 p) { return DesertWeight(p) >= .5f; }
}
