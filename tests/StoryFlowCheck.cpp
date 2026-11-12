// Run from SimpleGame; link the gameplay modules including StorySystem and GameContent.
// Source only until the user explicitly requests a build/test run.
#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/StorySystem.h"
#include "../SimpleGame/CraftingSystem.h"
#include "../SimpleGame/GameUI.h"
#include "../SimpleGame/Chapter1BoarAnimation.h"
#include <cassert>
#include <iostream>

int main()
{
    using namespace Chapter1BoarAnimation;
    for (Action action : {Attack, Hit, Death})
    {
        assert(Frame(action, 0) == 0);
        int elapsed = 0;
        for (int frame = 0; frame < 6; ++frame)
        {
            assert(Frame(action, (elapsed + FrameMilliseconds[action][frame] * .5f) / 1000) == frame);
            elapsed += FrameMilliseconds[action][frame];
        }
        assert(Frame(action, Duration(action) + 10) == 5);
    }
    auto boss = MonsterSystem::Create(GameContent::Get().monsters.at("rampaging_boar"), {0, 0});
    float playerHealth = 100;
    assert(MonsterSystem::UpdateChaseAndAttack(boss, {0, 0}, 0, 0, playerHealth, 0));
    assert(boss.attackAnimation == Duration(Attack));
    assert(playerHealth == 100 - boss.definition.damage);
    assert(MonsterSystem::Hit(boss, {0, 0}, 1).hit);
    assert(boss.hitAnimation == Duration(Hit) && boss.attackAnimation == 0);
    MonsterSystem::UpdateTimers(boss, .5f);
    assert(boss.hitAnimation == 0);
    assert(MonsterSystem::Hit(boss, {0, 0}, boss.health).defeated);
    const Vec2 deathFacing = boss.animationFacing;
    MonsterSystem::UpdateTimers(boss, 1.1f);
    assert(Frame(Death, boss.deathTime) == 5);
    assert(!MonsterSystem::UpdateChaseAndAttack(boss, {3, 3}, 0, 0, playerHealth, 0));
    assert(boss.animationFacing.x == deathFacing.x && boss.animationFacing.z == deathFacing.z);
    auto otherBoss = MonsterSystem::Create(GameContent::Get().monsters.at("displaced_boar"), {0, 0});
    MonsterSystem::Hit(otherBoss, {0, 0}, 1);
    assert(otherBoss.hitAnimation == 0 && otherBoss.attackAnimation == 0);

    // Debug entry and save-cursor restoration are deliberately different operations.
    for (const auto &step : GameContent::Get().story.steps)
    {
        GameState debug = CreateInitialGameState();
        debug.width = 1920; debug.height = 1080; debug.time = 12.0f;
        assert(StorySystem::DebugEnterStep(debug, step.id));
        assert(StorySystem::Current(debug)->id == step.id);
        assert(!debug.storyFinished && debug.quest == QuestStage::Complete && debug.hunterBladeCrafted);
        assert(debug.width == 1920 && debug.height == 1080 && debug.time == 12.0f);
        assert(!debug.inventoryOpen && debug.health == debug.maxHealth);
        GameManager debugGame(debug);
        const int before = debug.storyStep;
        switch (step.action)
        {
        case GameContent::StoryAction::Travel:
            debugGame.UpdateGame(0);
            assert(debug.storyStep == before);
            debug.player = step.position;
            debugGame.UpdateGame(0);
            break;
        case GameContent::StoryAction::Investigate:
        case GameContent::StoryAction::Report:
        case GameContent::StoryAction::Collect:
            debugGame.HandleCommand(GameCommand::Interact);
            break;
        case GameContent::StoryAction::CraftArmor:
            assert(!debug.hunterArmorCrafted && debug.armorRecipeUnlocked);
            debugGame.HandleCommand(GameCommand::CraftHunterArmor);
            assert(debug.hunterArmorCrafted);
            break;
        case GameContent::StoryAction::Hunt:
            assert(debug.monster.definition.id == step.monsterId && debug.monster.health > 0);
            debug.monster.health = 1;
            debugGame.HandleCommand(GameCommand::Attack);
            break;
        }
        assert(debug.storyStep == before + 1);
    }
    for (const auto &chapter : GameContent::Get().story.chapters)
    {
        GameState debug = CreateInitialGameState();
        assert(StorySystem::DebugEnterChapter(debug, chapter.id));
        const auto *first = StorySystem::Current(debug);
        assert(first && first->chapterId == chapter.id);
        for (int index = 0; index < debug.storyStep; ++index)
            assert(GameContent::Get().story.steps[index].chapterId != chapter.id);
    }
    GameState saved = CreateInitialGameState();
    saved.health = 47; saved.attackPower = 19; saved.monster.health = 7;
    saved.inventory.Add(ItemId::Herb, 4);
    const auto &first = GameContent::Get().story.steps.front();
    assert(StorySystem::SetProgressById(saved, first.chapterId, first.id));
    assert(saved.health == 47 && saved.attackPower == 19 && saved.monster.health == 7);
    assert(saved.inventory.Count(ItemId::Herb) == 4 && !saved.hunterBladeCrafted);
    const int cursor = saved.storyStep;
    assert(!StorySystem::SetProgressById(saved, "missing", first.id));
    assert(!StorySystem::SetProgressById(saved, first.chapterId, "missing"));
    assert(!StorySystem::DebugEnterStep(saved, "missing"));
    assert(!StorySystem::DebugEnterChapter(saved, "missing"));
    assert(saved.storyStep == cursor && saved.health == 47 && saved.monster.health == 7);

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
