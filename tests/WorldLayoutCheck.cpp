#include "../SimpleGame/WorldLayout.h"
#include "../SimpleGame/PlayerController.h"
#include <cassert>
#include <iostream>

int main()
{
    for (const auto &strip : GameContent::Get().map.terrain)
    {
        const Vec2 mid={(strip.start.x+strip.end.x)*.5f,(strip.start.z+strip.end.z)*.5f};
        assert(std::abs(TerrainGeometry::Height(strip,mid)-strip.height)<.001f);
        assert(std::abs(TerrainGeometry::Height(strip,strip.start))<.001f && std::abs(TerrainGeometry::Height(strip,strip.end))<.001f);
        assert(!TerrainGeometry::Blocked(strip,mid));
        const float length=TerrainGeometry::Length(strip);
        const Vec2 side={mid.x-(strip.end.z-strip.start.z)/length*(strip.halfWidth+3),
            mid.z+(strip.end.x-strip.start.x)/length*(strip.halfWidth+3)};
        if (strip.kind==0) assert(TerrainGeometry::Height(strip,side)>0 && !TerrainGeometry::Blocked(strip,side));
        else assert(TerrainGeometry::Blocked(strip,side));
    }
    assert(!WorldLayout::IsBlocked({0, -2.5f}));
    assert(!WorldLayout::IsBlocked({0, 1.4f}));
    for (const auto &house : WorldLayout::Houses())
        assert(WorldLayout::IsBlocked(house.position));
    for (const auto &tree : WorldLayout::Trees())
        assert(WorldLayout::IsBlocked(tree.position));
    for (const auto &item : WorldLayout::Landmarks())
        if (item.radius > 0)
            assert(WorldLayout::IsBlocked(item.position));

    GameState state = CreateInitialGameState();
    state.keys['w'] = state.keys['a'] = true; // Along world +Z toward the elder.
    for (int i = 0; i < 11; ++i)
        PlayerController::UpdateMovement(state, .1f);
    assert(std::abs(state.player.x) < .001f && state.player.z > 1.2f);
    const Vec2 house = WorldLayout::Houses()[0].position;
    state.keys = {};
    state.keys['w'] = state.keys['d'] = true;
    state.player = {house.x-1.8f, house.z};
    const Vec2 before = state.player;
    PlayerController::UpdateMovement(state, .1f);
    assert(state.player.x == before.x && state.player.z == before.z);
    state.keys = {};
    state.keys['s'] = state.keys['a'] = true;
    PlayerController::UpdateMovement(state, .1f);
    assert(state.player.x < before.x);
    std::cout << "PASS: spawn and elder route clear, shared house/tree/landmark blockers, collision and retreat\n";
}
