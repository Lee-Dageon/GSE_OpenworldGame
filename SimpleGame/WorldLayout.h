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
inline bool CanMove(const WorldState &world,Vec2 from,Vec2 to,float radius=.18f)
{
    if (to.x-radius<world.minimumCoordinate || to.z-radius<world.minimumCoordinate ||
        to.x+radius>world.maximumCoordinate || to.z+radius>world.maximumCoordinate) return false;
    if (std::hypot(to.x-world.lake.x,to.z-world.lake.z)<world.lakeRadius+radius) return false;
    if (std::abs(GroundHeight(to)-GroundHeight(from))>.65f) return false;
    if (IsBlocked(to)) return false;
    for (int i=0;i<8;++i)
    {
        const float angle=i*.785398163f;
        if (IsBlocked({to.x+std::cos(angle)*radius,to.z+std::sin(angle)*radius})) return false;
    }
    return true;
}
inline bool ClearLine(Vec2 from,Vec2 to)
{
    const int steps=std::max(1,static_cast<int>(std::ceil(std::hypot(to.x-from.x,to.z-from.z)/.15f)));
    for (int i=1;i<steps;++i)
        if (IsBlocked({from.x+(to.x-from.x)*i/steps,from.z+(to.z-from.z)*i/steps})) return false;
    return true;
}
}
