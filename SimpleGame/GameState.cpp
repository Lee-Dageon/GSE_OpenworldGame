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
    {
        auto initial=npc.initial;
        if (npc.id.size()==6 && npc.id.compare(0,4,"npc_")==0 &&
            npc.id[4]>='0' && npc.id[4]<='9' && npc.id[5]>='0' && npc.id[5]<='9')
            initial.identity=(npc.id[4]-'0')*10+npc.id[5]-'0';
        state.npcs.push_back(initial);
    }
    for (const auto &animal : content.map.animals)
        state.animals.push_back(animal.initial);
    for (const auto &node : content.map.gatherNodes)
        state.gatherNodes.push_back(node.initial);
    return state;
}
