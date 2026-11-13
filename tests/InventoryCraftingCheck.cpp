#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/CraftingSystem.h"
#include "../SimpleGame/QuestSystem.h"
#include "../SimpleGame/GameUI.h"
#include "../SimpleGame/GameConstants.h"
#include <cassert>
#include <climits>
#include <cmath>
#include <limits>
#include <iostream>

int main()
{
    GameState zoom;
    GameInput::OnScroll(zoom, .5);
    assert(zoom.cameraZoom > 1 && zoom.cameraZoom < GameConstants::World::ZoomStep);
    zoom.cameraZoom = 1;
    GameInput::OnScroll(zoom, 1);
    assert(zoom.cameraZoom > 1);
    GameInput::OnScroll(zoom, -1);
    assert(std::abs(zoom.cameraZoom - 1) < .00001f);
    GameInput::OnScroll(zoom, 1000);
    assert(zoom.cameraZoom == GameConstants::World::ZoomMaximum);
    GameInput::OnScroll(zoom, -1000);
    assert(zoom.cameraZoom == GameConstants::World::ZoomMinimum);
    const float heldZoom = zoom.cameraZoom;
    zoom.inventoryOpen = true;
    GameInput::OnScroll(zoom, 1);
    assert(zoom.cameraZoom == heldZoom);
    zoom.inventoryOpen = false;
    GameInput::OnScroll(zoom, std::numeric_limits<double>::infinity());
    GameInput::OnScroll(zoom, std::numeric_limits<double>::quiet_NaN());
    assert(zoom.cameraZoom == heldZoom);
    Inventory bag;
    assert(bag.Count(ItemId::Potion) == 2);
    assert(!bag.Add(ItemId::Herb, -1) && !bag.Remove(ItemId::Potion, -1));
    assert(!bag.Add(static_cast<ItemId>(-1)) && !bag.Remove(ItemId::Count));
    assert(!bag.Add(ItemId::Potion, INT_MAX));
    assert(bag.Add(ItemId::Herb, 99) && !bag.Add(ItemId::Herb));
    assert(!bag.Remove(ItemId::Herb, 100) && bag.Count(ItemId::Herb) == 99);

    GameState failure;
    GameManager failureGame(failure);
    failure.player = failure.world.elder;
    failure.inventory.Add(ItemId::BoarHide);
    failure.inventory.Add(ItemId::BoarTusk);
    CraftingSystem::TryCraft(failure, CraftingSystem::Recipe::HunterBlade);
    assert(!failure.hunterBladeCrafted && failure.inventory.Count(ItemId::BoarTusk) == 1);
    failure.quest = QuestStage::Complete;
    failure.player = {50, 0};
    CraftingSystem::TryCraft(failure, CraftingSystem::Recipe::HunterBlade);
    assert(!failure.hunterBladeCrafted && failure.inventory.Count(ItemId::BoarHide) == 1);
    failure.player = failure.world.elder;
    failure.inventory.Remove(ItemId::BoarHide);
    CraftingSystem::TryCraft(failure, CraftingSystem::Recipe::HunterBlade);
    assert(!failure.hunterBladeCrafted && failure.inventory.Count(ItemId::BoarTusk) == 1);
    failure.inventory.Add(ItemId::Herb);
    failure.inventory.Add(ItemId::Potion, 97);
    failureGame.HandleCommand(GameInput::OnKeyDown(failure, 'p', false));
    assert(failure.inventory.Count(ItemId::Herb) == 1 && failure.inventory.Count(ItemId::Potion) == 99);
    assert(GameInput::OnKeyDown(failure, 'p', false) == GameCommand::None);
    GameInput::OnKeyUp(failure, 'p');
    failure.inventory.Remove(ItemId::Potion);
    failureGame.HandleCommand(GameInput::OnKeyDown(failure, 'P', false));
    assert(failure.inventory.Count(ItemId::Herb) == 0 && failure.inventory.Count(ItemId::Potion) == 99);
    failure.health = 50;
    failureGame.HandleCommand(GameCommand::UsePotion);
    assert(failure.health == 95 && failure.inventory.Count(ItemId::Potion) == 98);
    failure.health = failure.maxHealth;
    failureGame.HandleCommand(GameCommand::UsePotion);
    assert(failure.inventory.Count(ItemId::Potion) == 98);

    GameState state = CreateInitialGameState();
    GameManager game(state);
    state.player = state.world.elder;
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Tracks);
    state.player = state.world.tracks;
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Herb);
    state.player = state.world.herb;
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Hunt && state.inventory.Count(ItemId::Herb) == 1);
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.inventory.Count(ItemId::Herb) == 1);
    const auto kill = [&]() {
        state.player = state.monster.position;
        int attacks = 0;
        while (MonsterSystem::IsAlive(state.monster))
        {
            game.BeginFrame(.33f);
            game.HandleCommand(GameCommand::Attack);
            assert(++attacks < 100);
        }
        assert(state.quest == QuestStage::Trophy);
        return attacks;
    };
    assert(kill() == 6);
    const int xp = state.experience;
    game.BeginFrame(.33f);
    game.HandleCommand(GameCommand::Attack);
    assert(state.experience == xp);
    state.inventory.Add(ItemId::BoarHide, 99);
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Trophy && state.inventory.Count(ItemId::BoarTusk) == 0);
    state.inventory.Remove(ItemId::BoarHide, 99);
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Return && state.completedHunts == 1);
    assert(state.inventory.Count(ItemId::BoarHide) == 1 && state.inventory.Count(ItemId::BoarTusk) == 1);
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.inventory.Count(ItemId::BoarHide) == 1);
    state.player = state.world.elder;
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Complete);
    game.HandleCommand(GameInput::OnKeyDown(state, 'i', false));
    assert(state.inventoryOpen);
    assert(GameInput::OnKeyDown(state, 'i', false) == GameCommand::None);
    const float time = state.time;
    const Vec2 position = state.player;
    state.keys['w'] = true;
    game.UpdateGame(.1f);
    assert(state.time == time && state.player.x == position.x && state.player.z == position.z);
    game.HandleCommand(GameCommand::Attack);
    const int power = state.attackPower;
    game.HandleCommand(GameInput::OnKeyDown(state, 'C', false));
    assert(state.hunterBladeCrafted && state.attackPower == power+3);
    assert(GameInput::OnKeyDown(state, 'c', false) == GameCommand::None);
    assert(state.inventory.Count(ItemId::BoarHide) == 0 && state.inventory.Count(ItemId::BoarTusk) == 0);
    game.HandleCommand(GameCommand::CraftHunterBlade);
    assert(state.attackPower == power+3);
    game.HandleCommand(GameInput::OnKeyDown(state, 27, false));
    assert(!state.inventoryOpen);
    assert(GameInput::OnKeyDown(state, 27, false) == GameCommand::None);
    GameInput::OnKeyUp(state, 27);
    assert(GameInput::OnKeyDown(state, 27, false) == GameCommand::Quit);
    state.storyFinished = true; // This check covers post-story repeat hunts; StoryFlowCheck covers the campaign.
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Hunt && state.monster.health == 18);
    assert(kill() == 3);
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.completedHunts == 2 && state.quest == QuestStage::Return);
    state.player = state.world.elder;
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Complete && state.hunterBladeCrafted);
    for (auto size : {std::array<int,2>{640,360}, {1280,720}, {1920,1080}, {2560,1440}})
    {
        for (const auto &button : InventoryButtons(size[0], size[1]))
        {
            assert(button.x >= 0 && button.y >= 0 && button.x+button.width < size[0] && button.y+button.height < size[1]);
            assert(HitTestGameUi(static_cast<int>(button.x+button.width/2),
                static_cast<int>(button.y+button.height/2), size[0], size[1], true) == button.command);
        }
        const auto entry = InventoryEntry(size[0], size[1]);
        assert(HitTestGameUi(static_cast<int>(entry.x+entry.width/2), static_cast<int>(entry.y+entry.height/2),
            size[0], size[1]) == GameCommand::ToggleInventory);
        assert(HitTestGameUi(1,1,size[0],size[1],true) == GameCommand::None);
    }
    std::cout << "PASS: stack bounds, atomic failure, potion recipe, full tutorial, loot once, craft/equip once, pause/input, stronger repeat hunt and UI hit tests\n";
}
