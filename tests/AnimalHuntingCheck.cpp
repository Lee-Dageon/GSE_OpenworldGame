#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/BoarDeathFrames.h"
#include "../SimpleGame/LootTable.h"
#include "../SimpleGame/AnimalHitFrames.h"
#include "../SimpleGame/WildlifeSystem.h"
#include <cassert>

int main()
{
    assert(AnimalHitFrames::Pose(GameConstants::Feedback::MonsterFlashDuration) == 0);
    assert(AnimalHitFrames::Pose(.001f) == 3);
    Inventory full;
    full.Add(ItemId::Antler, Inventory::MaxStack);
    const auto before = full.stacks;
    assert(!LootTable::Grant(full, LootTable::Animal(0)) && full.stacks == before);
    MonsterDefinition guardian;
    guardian.id = "ancient_guardian"; guardian.hideDrop = 4; guardian.tuskDrop = 3;
    Inventory reward;
    assert(LootTable::Grant(reward, LootTable::Monster(guardian)));
    assert(reward.Count(ItemId::GuardianScale) == 2 && reward.Count(ItemId::AncientCrystal) == 1);
    assert(reward.Count(ItemId::RawMeat) == 0);
    assert(BoarDeathFrames::Pose(-1) == 0 && BoarDeathFrames::Pose(0) == 0);
    assert(BoarDeathFrames::Pose(.21f) == 1 && BoarDeathFrames::Pose(100) == 3);
    for (int species=0;species<5;++species)
    {
        auto state=CreateInitialGameState();
        AnimalState animal={}; animal.column=species;
        animal.position=animal.home={state.player.x+1,state.player.z};
        animal.health=WildlifeSystem::Health(species);
        const auto before=animal.position;
        assert(WildlifeSystem::Hit(animal,state.player,2)==2 && animal.fleeTime>0);
        assert(WildlifeSystem::Hit(animal,state.player,2)==0);
        WildlifeSystem::Update(animal,state,.1f);
        assert(animal.position.x-before.x>.3f); // Keeps fleeing during hit flash.
        assert(animal.position.x-before.x<GameConstants::Player::SprintSpeed*.1f);
        state.player={800,800}; animal.fleeTime=0;
        const auto fleeing=animal.position;
        WildlifeSystem::Update(animal,state,.1f);
        assert(std::hypot(animal.position.x-fleeing.x,animal.position.z-fleeing.z)<=.081f);
    }
    assert(LootTable::Animal(3)[static_cast<size_t>(ItemId::LizardScale)]==2);
    assert(LootTable::Animal(3)[static_cast<size_t>(ItemId::BoarTusk)]==0);
    assert(LootTable::Animal(4)[static_cast<size_t>(ItemId::CamelMilk)]==1);
    assert(LootTable::Animal(4)[static_cast<size_t>(ItemId::CamelHide)]==2);
    for (int species=0;species<3;++species)
    {
        auto state=CreateInitialGameState();
        state.animals.clear();
        AnimalState animal={};
        animal.position=animal.home=state.player;
        animal.column=species;
        animal.health=1;
        state.animals.push_back(animal);
        GameManager game(state);
        const auto quest=state.quest;
        game.HandleCommand(GameCommand::Attack);
        assert(state.animals[0].health==0 && state.combo==1);
        assert(state.animals[0].deathTime == 0);
        assert(state.damageNumbers.back().damage==1 && state.quest==quest);
        game.UpdateGame(.2f);
        assert(state.animals[0].deathTime > 0);
        state.inventoryOpen = true;
        const float pausedDeath = state.animals[0].deathTime;
        game.UpdateGame(1);
        assert(state.animals[0].deathTime == pausedDeath);
        state.inventoryOpen = false;
        game.HandleCommand(GameCommand::Interact);
        assert(state.lootAnimal==0 && state.inventory.Count(ItemId::BoarHide)==0);
        game.HandleCommand(GameCommand::Interact);
        game.UpdateGame(GameConstants::Loot::Duration+.01f);
        const int hides = species == 2 ? 0 : 1;
        assert(state.animals[0].collected && state.inventory.Count(ItemId::BoarHide)==hides);
        assert(state.inventory.Count(ItemId::BoarTusk)==(species==1 ? 1 : 0));
        assert(state.inventory.Count(ItemId::RawMeat)==(species==2 ? 1 : 2));
        assert(state.inventory.Count(ItemId::Antler)==(species==0 ? 1 : 0));
        assert(state.inventory.Count(ItemId::Wool)==(species==2 ? 2 : 0));
        assert(state.quest==quest && state.completedHunts==0);
        game.HandleCommand(GameCommand::Interact);
        assert(state.inventory.Count(ItemId::BoarHide)==hides);
        game.UpdateGame(GameConstants::World::AnimalRespawnSeconds+.1f);
        assert(!state.animals[0].collected && state.animals[0].health==WildlifeSystem::Health(species));
        assert(state.animals[0].deathTime == 0);
        state.animals[0].health=0;
        state.player=state.animals[0].position;
        state.inventory.Add(ItemId::RawMeat,Inventory::MaxStack-state.inventory.Count(ItemId::RawMeat));
        const auto preserved = state.inventory.stacks;
        game.HandleCommand(GameCommand::Interact);
        game.UpdateGame(GameConstants::Loot::Duration+.01f);
        assert(!state.animals[0].collected && state.inventory.stacks==preserved);
    }
}
