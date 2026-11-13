// Run from SimpleGame; link the gameplay modules including StorySystem and GameContent.
// Source only until the user explicitly requests a build/test run.
#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/StorySystem.h"
#include "../SimpleGame/CraftingSystem.h"
#include "../SimpleGame/GameUI.h"
#include "../SimpleGame/Chapter1BoarAnimation.h"
#include "../SimpleGame/DamageFeedback.h"
#include "../SimpleGame/GameConstants.h"
#include "../SimpleGame/BossIntro.h"
#include "../SimpleGame/ChatacabraFrames.h"
#include "../SimpleGame/PlayerController.h"
#include <cmath>
#include <cassert>
#include <iostream>

int main()
{
    GameState cinematic = CreateInitialGameState();
    cinematic.cameraZoom = 1.3f;
    assert(StorySystem::DebugEnterStep(cinematic, "ancient_hunt"));
    assert(BossIntro::Active(cinematic) && BossIntro::Weight(cinematic) == 0);
    GameManager cinematicGame(cinematic);
    const auto playerBefore = cinematic.player;
    const float timeBefore = cinematic.time, healthBefore = cinematic.health;
    const int bossBefore = cinematic.monster.health;
    cinematicGame.HandleCommand(GameCommand::Attack);
    cinematicGame.HandleCommand(GameCommand::ToggleInventory);
    GameInput::OnScroll(cinematic, 1);
    assert(cinematic.monster.health == bossBefore && !cinematic.inventoryOpen && cinematic.cameraZoom == 1.3f);
    cinematicGame.UpdateGame(2);
    assert(cinematic.time == timeBefore && cinematic.health == healthBefore);
    assert(cinematic.player.x == playerBefore.x && cinematic.player.z == playerBefore.z);
    assert(cinematic.monster.chatPattern == -1 && BossIntro::Weight(cinematic) == 1);
    assert(BossIntro::Zoom(cinematic) >= GameConstants::Chatacabra::IntroZoom &&
        BossIntro::Zoom(cinematic) <= GameConstants::Chatacabra::IntroZoomClose);
    const float wideZoom = BossIntro::Zoom(cinematic);
    cinematicGame.UpdateGame(4);
    assert(BossIntro::Active(cinematic) && BossIntro::Zoom(cinematic) > wideZoom);
    assert(BossIntro::FootOffset(cinematic) > 86);
    cinematicGame.UpdateGame(GameConstants::Chatacabra::IntroDuration);
    assert(!BossIntro::Active(cinematic) && BossIntro::Zoom(cinematic) == 1.3f);
    assert(BossIntro::Focus(cinematic).x == cinematic.player.x && BossIntro::Focus(cinematic).z == cinematic.player.z);
    assert(cinematic.time == timeBefore && cinematic.health == healthBefore);
    for (unsigned char key : {static_cast<unsigned char>(27), static_cast<unsigned char>(' ')})
    {
        GameState skipped = CreateInitialGameState();
        assert(StorySystem::DebugEnterStep(skipped, "ancient_hunt"));
        assert(GameInput::OnKeyDown(skipped, key, false) == GameCommand::None);
        assert(!BossIntro::Active(skipped) && skipped.dodge == 0 && skipped.cameraZoom == 1);
    }
    assert(BossIntro::SlamPose(0) == -1);
    assert(BossIntro::SlamPose(GameConstants::Chatacabra::IntroPrepareTime) == 0);
    assert(BossIntro::SlamPose(GameConstants::Chatacabra::IntroRaiseTime) == 2);
    assert(BossIntro::SlamPose(GameConstants::Chatacabra::IntroImpactTime) == 3);
    assert(BossIntro::SlamPose(GameConstants::Chatacabra::IntroRecoverTime) == -1);
    GameState victory = CreateInitialGameState();
    assert(StorySystem::DebugEnterStep(victory, "ancient_hunt"));
    BossIntro::Skip(victory);
    victory.cameraZoom = 1.3f;
    victory.monster.health = 1;
    victory.player = victory.monster.position;
    GameManager victoryGame(victory);
    victoryGame.HandleCommand(GameCommand::Attack);
    assert(victory.monster.health == 0 && victory.bossDefeatIntro && BossIntro::Active(victory));
    assert(StorySystem::HasDrop(victory));
    const int victoryStep = victory.storyStep, victoryExperience = victory.experience;
    const int scalesBefore = victory.inventory.Count(ItemId::GuardianScale);
    const float victoryTime = victory.time, victoryHealth = victory.health;
    for (GameCommand command : {GameCommand::Attack, GameCommand::Interact, GameCommand::ToggleInventory})
        victoryGame.HandleCommand(command);
    GameInput::OnScroll(victory, 1);
    assert(!victory.inventoryOpen && victory.cameraZoom == 1.3f && victory.lootTime == 0);
    victoryGame.UpdateGame(1);
    assert(ChatacabraFrames::DeathPose(victory.monster.deathTime) == 0);
    victoryGame.UpdateGame(1);
    assert(ChatacabraFrames::DeathPose(victory.monster.deathTime) == 1);
    victoryGame.UpdateGame(1);
    assert(ChatacabraFrames::DeathPose(victory.monster.deathTime) == 2);
    victoryGame.UpdateGame(1);
    assert(ChatacabraFrames::DeathPose(victory.monster.deathTime) == 3 && BossIntro::Active(victory));
    assert(victory.time == victoryTime && victory.health == victoryHealth && victory.storyStep == victoryStep);
    assert(victory.experience == victoryExperience && victory.inventory.Count(ItemId::GuardianScale) == scalesBefore);
    victoryGame.UpdateGame(GameConstants::Chatacabra::DefeatDuration);
    assert(!BossIntro::Active(victory) && BossIntro::Zoom(victory) == 1.3f);
    victoryGame.HandleCommand(GameCommand::Interact);
    victoryGame.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(victory.inventory.Count(ItemId::GuardianScale) == scalesBefore + 2);
    victoryGame.HandleCommand(GameCommand::Interact);
    assert(victory.inventory.Count(ItemId::GuardianScale) == scalesBefore + 2);
    for (unsigned char key : {static_cast<unsigned char>(27), static_cast<unsigned char>(' ')})
    {
        GameState skipped = CreateInitialGameState();
        assert(StorySystem::DebugEnterStep(skipped, "ancient_hunt"));
        BossIntro::Skip(skipped);
        skipped.monster.health = 1;
        GameManager skippedGame(skipped);
        skippedGame.HandleCommand(GameCommand::Attack);
        assert(BossIntro::Active(skipped) && skipped.bossDefeatIntro);
        GameInput::OnKeyDown(skipped, key, false);
        assert(!BossIntro::Active(skipped) && skipped.dodge == 0);
        assert(ChatacabraFrames::DeathPose(skipped.monster.deathTime) == 3 && StorySystem::HasDrop(skipped));
    }
    auto gatherer=CreateInitialGameState();
    gatherer.quest=QuestStage::Complete;
    gatherer.player=gatherer.world.herb;
    GameManager gathering(gatherer);
    gathering.HandleCommand(GameCommand::Interact);
    assert(gatherer.inventory.Count(ItemId::Herb)==1 && gatherer.herbRegrowTime>0);
    gathering.HandleCommand(GameCommand::Interact);
    assert(gatherer.inventory.Count(ItemId::Herb)==1);
    gathering.UpdateGame(GameConstants::Loot::HerbRegrowSeconds+.1f);
    gatherer.player=gatherer.world.herb;
    gathering.HandleCommand(GameCommand::Interact);
    assert(gatherer.inventory.Count(ItemId::Herb)==2);
    GameState looter = CreateInitialGameState();
    looter.quest = QuestStage::Trophy;
    looter.monster.health = 0;
    looter.monster.position = looter.player;
    GameManager lootGame(looter);
    const int initialHide = looter.inventory.Count(ItemId::BoarHide);
    lootGame.HandleCommand(GameCommand::Interact);
    assert(looter.lootTime > 0 && looter.inventory.Count(ItemId::BoarHide) == initialHide);
    lootGame.HandleCommand(GameCommand::Interact);
    lootGame.UpdateGame(GameConstants::Loot::Duration * .4f);
    assert(looter.inventory.Count(ItemId::BoarHide) == initialHide);
    looter.inventoryOpen = true;
    const float pausedLoot = looter.lootTime;
    lootGame.UpdateGame(1);
    assert(looter.lootTime == pausedLoot);
    looter.inventoryOpen = false;
    lootGame.UpdateGame(GameConstants::Loot::Duration);
    assert(looter.lootTime == 0 && looter.quest == QuestStage::Return);
    assert(looter.inventory.Count(ItemId::BoarHide) == initialHide + looter.monster.definition.hideDrop);
    GameState attacker = CreateInitialGameState();
    attacker.quest = QuestStage::Hunt;
    attacker.monster.position = attacker.player;
    attacker.monster.health = attacker.monster.maxHealth = 100;
    attacker.attackPower = 7;
    attacker.monster.definition.damage = 0;
    GameManager attackerGame(attacker);
    for (int count = 1; count <= 3; ++count)
    {
        attackerGame.HandleCommand(GameCommand::Attack);
        assert(attacker.combo == count && attacker.damageNumbers.back().damage == 3);
        assert(attacker.hitStop > 0 && attacker.attackHitTime > 0);
        attackerGame.UpdateGame(.5f);
    }
    const int comboBeforeMiss = attacker.combo;
    attacker.player.x += 50;
    attackerGame.HandleCommand(GameCommand::Attack);
    assert(attacker.combo == comboBeforeMiss);
    attackerGame.UpdateGame(2.1f);
    assert(attacker.combo == 0 && attacker.damageNumbers.empty());
    attacker.player = attacker.monster.position;
    attacker.monster.health = 3;
    attacker.attackPower = 100;
    attackerGame.HandleCommand(GameCommand::Attack);
    assert(attacker.damageNumbers.back().damage == 3 && attacker.combo == 1);
    attacker.quest = QuestStage::Hunt;
    attacker.monster.health = 10;
    attacker.attackPower = 1;
    attacker.attack = attacker.hitStop = 0;
    attackerGame.HandleCommand(GameCommand::Attack);
    assert(attacker.damageNumbers.back().damage == 1);

    GameState victim = CreateInitialGameState();
    victim.quest = QuestStage::Hunt;
    victim.monster.position = victim.player;
    GameManager victimGame(victim);
    victimGame.UpdateGame(0);
    assert(victim.playerHitTime == DamageFeedback::Duration && victim.hitStop == DamageFeedback::HitStop);
    assert(victim.playerSlowTime == GameConstants::Player::HitSlowDuration);
    const float frozenTime = victim.time, frozenHealth = victim.health;
    victimGame.UpdateGame(.03f);
    assert(victim.playerSlowTime == GameConstants::Player::HitSlowDuration);
    assert(victim.time == frozenTime && victim.health == frozenHealth && victim.hitStop > 0);
    victimGame.UpdateGame(.05f);
    assert(victim.time > frozenTime && victim.hitStop == 0);
    assert(victim.playerSlowTime < GameConstants::Player::HitSlowDuration);
    GameState walking = CreateInitialGameState();
    walking.keys['w'] = true;
    GameState slowed = walking;
    slowed.playerSlowTime = GameConstants::Player::HitSlowDuration;
    PlayerController::UpdateMovement(walking, .1f);
    PlayerController::UpdateMovement(slowed, .1f);
    const float normalDistance = walking.player.x;
    assert(normalDistance > 0 && std::abs(slowed.player.x / normalDistance - GameConstants::Player::HitSlowSpeed) < .0001f);
    walking = CreateInitialGameState(); walking.keys['w'] = true; walking.dodge = .1f;
    slowed = walking; slowed.playerSlowTime = GameConstants::Player::HitSlowDuration;
    PlayerController::UpdateMovement(walking, .01f);
    PlayerController::UpdateMovement(slowed, .01f);
    assert(walking.player.x == slowed.player.x && walking.player.z == slowed.player.z);
    victim.monster.definition.damage = 0;
    victimGame.BeginFrame(GameConstants::Player::HitSlowDuration + .1f);
    assert(victim.playerSlowTime == 0);
    victim.inventoryOpen = true;
    victim.playerSlowTime = .5f;
    const float pausedFeedback = victim.playerHitTime;
    victimGame.UpdateGame(1);
    assert(victim.playerHitTime == pausedFeedback && victim.playerSlowTime == .5f);
    for (bool dodge : {false, true})
    {
        GameState safe = CreateInitialGameState();
        safe.quest = QuestStage::Hunt;
        safe.monster.position = safe.player;
        if (dodge) safe.dodge = 1;
        else safe.monster.definition.damage = 0;
        GameManager(safe).UpdateGame(0);
        assert(safe.playerHitTime == 0 && safe.hitStop == 0 && safe.playerSlowTime == 0);
    }
    assert(DamageFeedback::Fade(0, DamageFeedback::BorderDuration) == 0);
    assert(DamageFeedback::Fade(DamageFeedback::Duration, DamageFeedback::BorderDuration) == 1);
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
        BossIntro::Skip(debug); // Progression checks bypass the separately tested presentation delay.
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
    if (debug.lootTime > 0) debugGame.UpdateGame(GameConstants::Loot::Duration + .01f);
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
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.storyStep == -1);
    state.player = state.world.elder;
    CraftingSystem::TryCraft(state,CraftingSystem::Recipe::HunterArmor);
    assert(!state.hunterArmorCrafted && state.defense == 0);
    game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.storyStep == 0);
    int steps = 0, hunts = 0;
    while (const auto *step = StorySystem::Current(state))
    {
        state.hitStop = 0; // This existing state-flow test skips presentation delays.
        BossIntro::Skip(state);
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
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
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
                state.hitStop = 0;
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
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
                assert(state.storyStep == before && state.inventory.Count(ItemId::BoarTusk) == tusk);
                state.inventory.Remove(ItemId::BoarHide,Inventory::MaxStack-hide);
            }
            const int hideReward = state.monster.definition.hideDrop, tuskReward = state.monster.definition.tuskDrop;
            game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
            assert(state.inventory.Count(ItemId::BoarHide) == hide+hideReward && state.inventory.Count(ItemId::BoarTusk) == tusk+tuskReward);
            game.HandleCommand(GameCommand::Interact);
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
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
    assert(steps == 22 && hunts == 3 && state.storyFinished && !StorySystem::CanStart(state));
    assert(state.ruinDeviceDisabled);
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
    if (state.lootTime > 0) game.UpdateGame(GameConstants::Loot::Duration + .01f);
    assert(state.quest == QuestStage::Hunt && !StorySystem::IsHunting(state));
    assert(state.monster.definition.id == GameContent::Get().map.monsterId);
    std::cout << "PASS: three chapters/22 objectives, device state, renewable herbs, loot/armor/pause and repeat hunt\n";
}
