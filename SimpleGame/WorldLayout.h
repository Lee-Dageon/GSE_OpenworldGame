#pragma once
#include "GameContent.h"

// Rendering and movement read the same immutable, validated map.
namespace WorldLayout
{
using SceneryKind = GameContent::SceneryKind;
inline const std::vector<GameContent::HouseDefinition> &Houses() { return GameContent::Get().map.houses; }
inline const std::vector<GameContent::TreeDefinition> &Trees() { return GameContent::Get().map.trees; }
inline const std::vector<GameContent::Scenery> &Landmarks() { return GameContent::Get().map.landmarks; }
inline bool IsBlocked(Vec2 position) { return GameContent::IsBlocked(GameContent::Get().map, position); }
}
