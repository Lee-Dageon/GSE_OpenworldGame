#include "../SimpleGame/WorldLayout.h"
#include "../SimpleGame/PlayerController.h"
#include <cassert>
#include <iostream>

int main()
{
    assert(!WorldLayout::IsBlocked({0, -2.5f}));
    assert(!WorldLayout::IsBlocked({0, 1.4f}));
    for (const Vec2 &house : WorldLayout::Houses)
        assert(WorldLayout::IsBlocked(house));
    for (const Vec2 &tree : WorldLayout::Trees)
        assert(WorldLayout::IsBlocked(tree));
    for (const auto &item : WorldLayout::Landmarks)
        if (item.radius > 0)
            assert(WorldLayout::IsBlocked(item.position));

    GameState state;
    state.keys['w'] = state.keys['a'] = true; // Along world +Z toward the elder.
    for (int i = 0; i < 11; ++i)
        PlayerController::UpdateMovement(state, .1f);
    assert(std::abs(state.player.x) < .001f && state.player.z > 1.2f);
    const Vec2 house = WorldLayout::Houses[0];
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
