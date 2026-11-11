// Run from SimpleGame; link the gameplay modules including StorySystem and GameContent.
// Source only until the user explicitly requests a build/test run.
#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/StorySystem.h"
#include "../SimpleGame/CraftingSystem.h"
#include "../SimpleGame/GameUI.h"
#include <cassert>
#include <iostream>

int main()
{
    GameState state = CreateInitialGameState();
    GameManager game(state);
    assert(!StorySystem::CanStart(state));
    state.quest = QuestStage::Complete;
    assert(!StorySystem::CanStart(state));
    state.hunterBladeCrafted = true;
    state.attackPower = 6;
    state.completedHunts = 1;
    assert(StorySystem::CanStart(state));
    state.player = state.world.spawn;
    game.HandleCommand(GameCommand::Interact);
    assert(state.storyStep == -1);
    state.player = state.world.elder;
    CraftingSystem::TryCraft(state,CraftingSystem::Recipe::HunterArmor);
    assert(!state.hunterArmorCrafted && state.defense == 0);
    game.HandleCommand(GameCommand::Interact);
    assert(state.storyStep == 0);
    int steps = 0, hunts = 0;
    while (const auto *step = StorySystem::Current(state))
    {
        assert(++steps <= 128);
        const int before = state.storyStep;
        state.player = StorySystem::Target(state);
        switch (step->action)
        {
        case GameContent::StoryAction::Travel:
            game.UpdateGame(0);
            break;
        case GameContent::StoryAction::Investigate:
        case GameContent::StoryAction::Report:
            game.HandleCommand(GameCommand::Interact);
            break;
        case GameContent::StoryAction::Hunt:
        {
            ++hunts;
            assert(state.monster.definition.id == step->monsterId && state.monster.health > 0);
            assert(state.monster.definition.spriteHeight > 125);
            for (int attack = 0; MonsterSystem::IsAlive(state.monster); ++attack)
            {
                assert(attack < 100);
                game.BeginFrame(.33f);
                game.HandleCommand(GameCommand::Attack);
            }
            assert(StorySystem::HasDrop(state));
            const int experience = state.experience;
            game.BeginFrame(.33f);
            game.HandleCommand(GameCommand::Attack);
            assert(state.experience == experience);
            break;
        }
        case GameContent::StoryAction::Collect:
        {
            const int hide = state.inventory.Count(ItemId::BoarHide), tusk = state.inventory.Count(ItemId::BoarTusk);
            if (hunts == 1)
            {
                state.inventory.Add(ItemId::BoarHide,Inventory::MaxStack-hide);
                game.HandleCommand(GameCommand::Interact);
                assert(state.storyStep == before && state.inventory.Count(ItemId::BoarTusk) == tusk);
                state.inventory.Remove(ItemId::BoarHide,Inventory::MaxStack-hide);
            }
            const int hideReward = state.monster.definition.hideDrop, tuskReward = state.monster.definition.tuskDrop;
            game.HandleCommand(GameCommand::Interact);
            assert(state.inventory.Count(ItemId::BoarHide) == hide+hideReward && state.inventory.Count(ItemId::BoarTusk) == tusk+tuskReward);
            game.HandleCommand(GameCommand::Interact);
            assert(state.inventory.Count(ItemId::BoarHide) == hide+hideReward);
            break;
        }
        case GameContent::StoryAction::CraftArmor:
        {
            assert(state.armorRecipeUnlocked);
            const int hide = state.inventory.Count(ItemId::BoarHide), tusk = state.inventory.Count(ItemId::BoarTusk);
            game.HandleCommand(GameCommand::ToggleInventory);
            game.HandleCommand(GameInput::OnKeyDown(state,'r',false));
            assert(state.hunterArmorCrafted && state.defense == 3 && state.inventoryOpen);
            assert(state.inventory.Count(ItemId::BoarHide) == hide-2 && state.inventory.Count(ItemId::BoarTusk) == tusk-1);
            game.HandleCommand(GameCommand::CraftHunterArmor);
            assert(state.defense == 3 && state.inventory.Count(ItemId::BoarHide) == hide-2);
            const int pausedStep = state.storyStep;
            state.player = StorySystem::Target(state);
            game.UpdateGame(1);
            assert(state.storyStep == pausedStep);
            game.HandleCommand(GameCommand::ToggleInventory);
            break;
        }
        }
        assert(state.storyStep == before+1);
    }
    assert(steps == 14 && hunts == 2 && state.storyFinished && !StorySystem::CanStart(state));
    const auto profile = GameContent::Get().monsters.at("displaced_boar");
    auto enemy = MonsterSystem::Create(profile,state.player);
    float health = 100;
    assert(MonsterSystem::UpdateChaseAndAttack(enemy,state.player,0,0,health,3));
    assert(health == 83);
    auto harmless = profile; harmless.damage = 0;
    enemy = MonsterSystem::Create(harmless,state.player);
    health = 100;
    MonsterSystem::UpdateChaseAndAttack(enemy,state.player,0,0,health,3);
    assert(health == 100);
    for (auto size : {std::array<int,2>{640,360},{1280,720},{1920,1080}})
    {
        const auto button = InventoryButtons(size[0],size[1])[2];
        assert(HitTestGameUi(static_cast<int>(button.x+button.width/2),static_cast<int>(button.y+button.height/2),
                             size[0],size[1],true) == GameCommand::CraftHunterArmor);
    }
    state.player = state.world.elder;
    game.HandleCommand(GameCommand::Interact);
    assert(state.quest == QuestStage::Hunt && !StorySystem::IsHunting(state));
    assert(state.monster.definition.id == GameContent::Get().map.monsterId);
    std::cout << "PASS: two chapters/14 objectives, prerequisites, clues, loot once/capacity, armor/pause, defense, button mapping and post-story repeat hunt\n";
}
