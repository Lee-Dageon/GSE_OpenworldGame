#include "stdafx.h"
#include "GameState.h"
#include "GameContent.h"

GameState CreateInitialGameState()
{
    const auto &content = GameContent::Get();
    GameState state;
    state.world = content.map.world;
    state.player = state.world.spawn;
    state.quest = content.quest.start;
    state.message = content.quest.welcome;
    state.monster = MonsterSystem::CreateEncounter();
    for (const auto &npc : content.map.npcs)
        state.npcs.push_back(npc.initial);
    for (const auto &animal : content.map.animals)
        state.animals.push_back(animal.initial);
    return state;
}
