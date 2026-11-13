#pragma once
#include "GameContent.h"

// Rendering and movement read the same immutable, validated map.
namespace WorldLayout
{
using SceneryKind = GameContent::SceneryKind;
inline const std::vector<GameContent::HouseDefinition> &Houses() { return GameContent::Get().map.houses; }
inline const std::vector<GameContent::TreeDefinition> &Trees() { return GameContent::Get().map.trees; }
inline const std::vector<GameContent::Scenery> &Landmarks() { return GameContent::Get().map.landmarks; }
inline float GroundHeight(Vec2 position)
{
    float height=0;
    for (const auto &strip : GameContent::Get().map.terrain)
        height=std::max(height,TerrainGeometry::Height(strip,position));
    return height;
}
inline bool IsBlocked(Vec2 position) { return GameContent::IsBlocked(GameContent::Get().map, position); }
}
