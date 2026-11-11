#include "stdafx.h"
#include "GameManager.h"

#include "Monster.h"
#include "PlayerController.h"
#include "QuestSystem.h"
#include "CraftingSystem.h"
#include "GameContent.h"
#include "StorySystem.h"

#include <algorithm>
#include <cmath>

GameManager::GameManager(GameState &state) : m_State(state) {}

void GameManager::BeginFrame(float deltaTime)
{
    m_State.time += deltaTime;
    m_State.dodge = std::max(0.0f, m_State.dodge - deltaTime);
    m_State.attack = std::max(0.0f, m_State.attack - deltaTime);
    m_State.messageTime = std::max(0.0f, m_State.messageTime - deltaTime);
    MonsterSystem::UpdateTimers(m_State.monster, deltaTime);
}

void GameManager::UpdateGame(float deltaTime)
{
    if (m_State.inventoryOpen)
    {
        m_State.moving = false;
        return;
    }
    BeginFrame(deltaTime);
    PlayerController::UpdateMovement(m_State, deltaTime);
    StorySystem::Update(m_State);
    for (AnimalState &animal : m_State.animals)
    {
        animal.phase += deltaTime * animal.speed;
        animal.position.x = animal.home.x + std::sin(animal.phase) * 2.4f;
        animal.position.z = animal.home.z + std::cos(animal.phase * .7f) * 1.8f;
    }
    if ((m_State.quest == QuestStage::Hunt || StorySystem::IsHunting(m_State)) &&
        MonsterSystem::UpdateChaseAndAttack(m_State.monster, m_State.player, m_State.dodge, deltaTime, m_State.health, m_State.defense))
        ShowMessage(L"멧돼지에게 공격받았습니다. 공백 키로 회피하거나 Q로 회복하세요.");
    if (m_State.health <= 0.0f)
    {
        m_State.player = m_State.world.spawn;
        m_State.health = m_State.maxHealth;
        ShowMessage(L"구조대의 도움으로 마을에 돌아왔습니다.");
    }
}

void GameManager::HandleCommand(GameCommand command)
{
    if (command == GameCommand::ToggleInventory)
    {
        m_State.inventoryOpen = !m_State.inventoryOpen;
        m_State.moving = false;
        return;
    }
    if (command == GameCommand::CraftHunterBlade || command == GameCommand::CraftHealingPotion || command == GameCommand::CraftHunterArmor)
    {
        const auto recipe = command == GameCommand::CraftHunterArmor ? CraftingSystem::Recipe::HunterArmor :
            (command == GameCommand::CraftHunterBlade ? CraftingSystem::Recipe::HunterBlade : CraftingSystem::Recipe::HealingPotion);
        ShowMessage(CraftingSystem::TryCraft(m_State,recipe));
        StorySystem::Update(m_State);
        return;
    }
    if (m_State.inventoryOpen && command != GameCommand::UsePotion)
        return;
    if (command == GameCommand::Interact)
    {
        if (StorySystem::Current(m_State) || StorySystem::CanStart(m_State))
        {
            if (const wchar_t *message = StorySystem::Interact(m_State)) ShowMessage(message);
            return;
        }
        const QuestSystem::InteractionResult result =
            QuestSystem::TryInteract(m_State, m_State.world.elder, m_State.world.tracks, m_State.world.herb);
        if (const wchar_t *message = QuestSystem::GetInteractionMessage(result))
            ShowMessage(message);
        return;
    }
    if (command == GameCommand::UsePotion && PlayerController::TryUsePotion(m_State))
    {
        ShowMessage(L"회복 물약을 사용했습니다.");
        return;
    }
    if (command == GameCommand::Dodge)
    {
        PlayerController::StartDodge(m_State);
        return;
    }
    if (command == GameCommand::Attack && m_State.attack <= 0.0f)
    {
        m_State.attack = .32f;
        if (m_State.quest != QuestStage::Hunt && !StorySystem::IsHunting(m_State))
            return;
        const MonsterHitResult result = MonsterSystem::Hit(m_State.monster, m_State.player, m_State.attackPower);
        if (!result.defeated)
            return;
        if (StorySystem::IsHunting(m_State))
        {
            QuestSystem::AddExperience(m_State,m_State.monster.definition.experience);
            if (const wchar_t *message = StorySystem::EnemyDefeated(m_State)) ShowMessage(message);
            return;
        }
        m_State.quest = GameContent::Step(m_State.quest).next;
        ShowMessage(QuestSystem::AddExperience(m_State, m_State.monster.definition.experience) ? L"레벨이 올랐습니다! 최대 체력과 공격력이 증가했습니다."
                                                          : GameContent::Step(QuestStage::Hunt).message.c_str());
    }
}

const wchar_t *GameManager::GetHint() const
{
    if (StorySystem::Current(m_State) || StorySystem::CanStart(m_State)) return StorySystem::Hint(m_State);
    return QuestSystem::GetHint(m_State);
}

void GameManager::ShowMessage(const wchar_t *message)
{
    m_State.message = message;
    m_State.messageTime = 5.0f;
}

void GameManager::UpdateSceneGraph(Actor &sceneRoot, float deltaTime)
{
    sceneRoot.Update(deltaTime);
}
