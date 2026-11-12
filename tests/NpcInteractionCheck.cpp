#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/NpcSystem.h"
#include <cassert>
#include <set>

int main()
{
    auto state=CreateInitialGameState();
    assert(state.npcs.size()==21);
    std::set<std::wstring> names;
    for (const auto &npc:state.npcs) names.insert(NpcSystem::Name(npc));
    assert(names.size()==21);
    assert(state.npcs.back().identity==20 && NpcSystem::Role(state.npcs.back())==5);
    assert(std::wstring(NpcSystem::Name(state.npcs.back()))==L"아린");
    auto &trader=state.npcs[3];
    state.inventory.Add(ItemId::BoarHide,2);
    const int potions=state.inventory.Count(ItemId::Potion);
    NpcSystem::Interact(state,trader);
    assert(state.inventory.Count(ItemId::Potion)==potions+1 && state.inventory.Count(ItemId::BoarHide)==0);
    state.inventory.Add(ItemId::BoarHide,2);
    state.inventory.Add(ItemId::Potion,Inventory::MaxStack-potions-1);
    NpcSystem::Interact(state,trader);
    assert(state.inventory.Count(ItemId::BoarHide)==2);
    auto &farmer=state.npcs[6];
    state.inventory.Add(ItemId::Herb,2);
    NpcSystem::Interact(state,farmer);
    assert(state.inventory.Count(ItemId::Herb)==2);
    state.health=state.maxHealth-5;
    NpcSystem::Interact(state,farmer);
    assert(state.health==state.maxHealth && state.inventory.Count(ItemId::Herb)==1);
    auto &smith=state.npcs[1];
    state.quest=QuestStage::Complete;
    state.player=smith.position;
    assert(CraftingSystem::IsAtWorkbench(state));
    state.inventory.Add(ItemId::BoarTusk);
    NpcSystem::Interact(state,smith);
    assert(state.hunterBladeCrafted);
    state.storyStep=0;
    state.player=state.npcs[4].position;
    const int conversations=state.npcs[4].conversations;
    GameManager game(state);
    game.HandleCommand(GameCommand::Interact);
    assert(state.npcs[4].conversations!=conversations); // Main story cannot swallow village dialogue.
    state.player=state.world.elder;
    assert(NpcSystem::Nearby(state)==-1);
}
