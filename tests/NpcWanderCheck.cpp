// Run from the runtime directory with Data present. Build/run only on explicit request.
#include "../SimpleGame/stdafx.h"
#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/WorldLayout.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
float Distance(Vec2 a, Vec2 b)
{
    return std::hypot(a.x - b.x, a.z - b.z);
}
}

int main()
{
    GameState state = CreateInitialGameState();
    GameManager manager(state);
    state.player = {-60, -60};
    const auto initial = state.npcs;
    std::vector<bool> moved(initial.size(), false);
    size_t elder = initial.size();
    for (size_t i = 0; i < initial.size(); ++i)
        if (Distance(initial[i].position, state.world.elder) < .1f)
            elder = i;
    assert(elder < initial.size());

    for (int frame = 0; frame < 2400; ++frame)
    {
        manager.UpdateGame(.05f);
        for (size_t i = 0; i < state.npcs.size(); ++i)
        {
            const auto &npc = state.npcs[i];
            moved[i] = moved[i] || Distance(npc.position, initial[i].position) > .2f;
            assert(Distance(npc.position, initial[i].position) <= 3.001f);
            assert(!WorldLayout::IsBlocked(npc.position));
            assert(Distance(npc.position, state.world.lake) > state.world.lakeRadius);
            assert(npc.position.x >= state.world.minimumCoordinate && npc.position.x <= state.world.maximumCoordinate);
            assert(npc.position.z >= state.world.minimumCoordinate && npc.position.z <= state.world.maximumCoordinate);
        }
        assert(Distance(state.npcs[elder].position, initial[elder].position) == 0);
    }
    size_t movedCount = 0;
    for (size_t i = 0; i < moved.size(); ++i)
        if (i != elder && moved[i]) ++movedCount;
    assert(movedCount == initial.size() - 1);

    const size_t villager = elder == 0 ? 1 : 0;
    const Vec2 before = state.npcs[villager].position;
    state.player = {before.x + .5f, before.z};
    manager.UpdateGame(.05f);
    assert(!state.npcs[villager].moving && Distance(before, state.npcs[villager].position) == 0);
    state.inventoryOpen = true;
    const auto paused = state.npcs;
    for (int frame = 0; frame < 20; ++frame) manager.UpdateGame(.05f);
    for (size_t i = 0; i < paused.size(); ++i)
    {
        assert(Distance(paused[i].position, state.npcs[i].position) == 0);
        assert(paused[i].phase == state.npcs[i].phase);
    }
    std::cout << "NPC wander, collision, elder, conversation distance and inventory pause passed\n";
}
