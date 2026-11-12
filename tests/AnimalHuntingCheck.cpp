#include "../SimpleGame/GameManager.h"
#include <cassert>

int main()
{
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
        assert(state.damageNumbers.back().damage==1 && state.quest==quest);
        game.UpdateGame(.2f);
        game.HandleCommand(GameCommand::Interact);
        assert(state.lootAnimal==0 && state.inventory.Count(ItemId::BoarHide)==0);
        game.HandleCommand(GameCommand::Interact);
        game.UpdateGame(GameConstants::Loot::Duration+.01f);
        assert(state.animals[0].collected && state.inventory.Count(ItemId::BoarHide)==1);
        assert(state.inventory.Count(ItemId::BoarTusk)==(species==1 ? 1 : 0));
        assert(state.quest==quest && state.completedHunts==0);
        game.HandleCommand(GameCommand::Interact);
        assert(state.inventory.Count(ItemId::BoarHide)==1);
        game.UpdateGame(GameConstants::World::AnimalRespawnSeconds+.1f);
        assert(!state.animals[0].collected && state.animals[0].health==GameConstants::World::AnimalHealth);
        state.animals[0].health=0;
        state.player=state.animals[0].position;
        state.inventory.Add(ItemId::BoarHide,Inventory::MaxStack-1);
        game.HandleCommand(GameCommand::Interact);
        game.UpdateGame(GameConstants::Loot::Duration+.01f);
        assert(!state.animals[0].collected && state.inventory.Count(ItemId::BoarHide)==Inventory::MaxStack);
    }
}
